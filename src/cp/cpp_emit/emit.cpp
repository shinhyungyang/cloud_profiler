#include "time_tsc.h"

#include "cloud_profiler.h"
#include "globals.h"
extern "C" {
#include "util.h"
//#include "net.h"
}

//#include "cxxopts.hpp"

//#include <errno.h>      // for using nanosleep

//#include <chrono>       // for using std::this_thread::sleep_for
#include <atomic>
#include <thread>
#include <pthread.h>

#include <sys/select.h> // for using select
#include <zmq.hpp>

#include <vector>
#include <boost/lockfree/spsc_queue.hpp>

#include <iostream>
#include <cstdlib> // for using aligned_malloc
#include <cassert>

#define MEBIBYTE              (1024 * 1024)
#define QUEUE_SIZE            (500 * 1000)
#define POLL_ENQUEUE_OVERHEAD (1900)          // 1900 ns per call @ 533 KT/s
#define TCP_BUF_SIZE          (MEBIBYTE * 12)
#define RECV_TIMEOUT          (0)
#define VAL_ZMQ_POLLITEMS     (1)
#define VAL_ZMQ_IMMEDIATE     (1)
#define TIME_BUDGET           (1000 * 1000)     // 1ms as a unit of emission
#define BKT_TIMEOUT           (TIME_BUDGET)     // 1ms for filling a bucket
#define BKT_TIMER_BUDGET      (BKT_TIMEOUT/10)  // one tenth of BKT_TIMEOUT

#define REPLAY_LIST_SIZE      (50*1000*90)   // assume 50 KT/s for 90 sec


///////////////////////////////////////////////////////////////////////////////
// DG Sender

struct sv_vars {
  int64_t sv_port_nr;
  int64_t emit_dur;
  int64_t pill_dur;
  int64_t tup_idx_offset;
  int64_t time_budget;
  int64_t record_dur;
  int64_t per_sec_throughput;
  int64_t slag;
  int64_t nr_threads;
  uint64_t replay_size;
  uint64_t remaining_replay_size;
  std::vector<std::vector<std::string *>*>* sarrays; // loaded string tuples
  std::vector<std::vector<std::vector<char>*>*>* barrays; // signed char (jbyte)
  std::vector<std::vector<std::string *>*>* s_pills; // poison pills (string)
  std::vector<std::vector<std::vector<char>*>*>* b_pills; // poison pills (jbyte)
  std::vector<std::thread*>* thrs;
  size_t thr_id;  // load_tuple()
  uint64_t iter;  // load_tuple()
  JavaAgentHandler* obj;  // get_tuple()
};

sv_vars g_svars;


///////////////////////////////////////////////////////////////////////////////
// DG Receiver

struct tcp_vars {
  std::vector<std::string>* sv_addresses;     // server ip:port addresses
  std::string sv_dotted_quad;                 // server ip address
  int32_t     sv_port_nr;                     // server port number
  bool        isServer;                       // run as a server or a client
  int64_t     nr_threads;                     // number of threads
  std::vector<boost::lockfree::spsc_queue<char*,
    boost::lockfree::capacity<QUEUE_SIZE>>*>* queues;

  zmq::context_t* ctx;
  zmq::socket_t* sock;
  zmq::pollitem_t* item;
  int64_t ch_fpc;
};

struct alignas(CACHELINE_SIZE) timer_vars {
  std::atomic<uint64_t> elapsed_intervals;
  std::atomic<bool> terminate;
  std::atomic<bool> terminated;
  std::thread* timer_thr __attribute__((aligned(CACHELINE_SIZE)));
};

struct alignas(CACHELINE_SIZE) timeout_vars {
  uint64_t next_timeout  __attribute__((aligned(CACHELINE_SIZE)));
};

struct alignas(CACHELINE_SIZE) zmq_vars {
  timer_vars* tm_vars __attribute__((aligned(CACHELINE_SIZE)));
  timeout_vars* tmo_vars __attribute__((aligned(CACHELINE_SIZE)));
  std::string* sv_address;  // server ip:port address
  zmq::context_t* ctx;      // ZMQ context
  zmq::socket_t* sock;      // ZMQ socket
  int64_t ch_fpc;           // 10MS-FPC channel
  int64_t ch_bid;           // Buffered ID channel
};

pthread_barrier_t barrier;

void st_run(int64_t ch);
  void        st_cleanup(int64_t ch);
  const char* st_recvStr(int64_t ch);

void mt_run(int64_t ch);
  void               mt_client(int64_t ch, int32_t tid, std::string address);
  void              mt_cleanup(int64_t ch);
  const char*       mt_recvStr(int64_t ch);
  const char*      mt_fetchStr(int64_t ch);
  template <typename T>
  void        mt_emitScheduler(int32_t tid, struct sv_vars* svvs,
                               std::vector<T>* v, std::vector<T>* v2);
  void           mt_selfFeeder(int64_t ch, int32_t tid, struct sv_vars* svvs);

void prepare_replays(JavaAgentHandler* obj,
                     uint64_t offset, uint64_t size,
                     std::vector<std::string *>* v);

void build_info(std::ostringstream * bi) {
  std::ostringstream dt;
  dt << "[" << __DATE__ << " " << __TIME__ << "] ";
  std::string mar(dt.str().size(), ' ');
  *bi << "\n";
  *bi << dt.str() << "v0.3.3-shyang-2" << "\n";
  *bi << mar      << "1. Object Factory (*)" << "\n";
  *bi << mar      << "2. Using lfence (*)" << "\n";
  *bi << mar      << "3. Unbounded tuple preloading (*)" << "\n";
  *bi << "\n";
}

// Current implementation is specific to ZMQ's PUSH/PULL pattern
int64_t openTCPChannel(bool isServer, std::vector<std::string> & addresses,
                       int64_t nr_threads) {
  tcp_vars* t_vars = new tcp_vars();
  int64_t ch = reinterpret_cast<intptr_t>(t_vars);

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  t_vars->sv_addresses = new std::vector<std::string>();
  for (std::vector<std::string>::size_type i=0; i<addresses.size(); ++i) {
    t_vars->sv_addresses->push_back(addresses[i]);
  }

  t_vars->isServer        = isServer;
  t_vars->nr_threads      = nr_threads;

//st_run(ch);
  mt_run(ch);

  return ch;
}

void closeTCPChannel(int64_t ch) {
//st_cleanup(ch);
//mt_cleanup(ch);
}

// per_sec_throughput:  tuples per second
// record_dur:          recording duration in nanoseconds
void initDGSender(JavaAgentHandler* obj,
                  int64_t sv_port_nr, int64_t emit_dur, int64_t pill_dur,
                  int64_t tup_idx_offset, int64_t record_dur,
                  int64_t per_sec_throughput, int64_t slag,
                  int64_t nr_threads) {

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  g_svars.sv_port_nr          = sv_port_nr;
  g_svars.emit_dur            = emit_dur;
  g_svars.pill_dur            = pill_dur;
  g_svars.tup_idx_offset      = tup_idx_offset;
  g_svars.record_dur          = record_dur;
  g_svars.per_sec_throughput  = per_sec_throughput;
  g_svars.slag                = slag;
  g_svars.nr_threads          = nr_threads;
  g_svars.time_budget         = TIME_BUDGET;
  g_svars.obj                 = obj;
  g_svars.iter                = 0;
  g_svars.thr_id              = 0;
  g_svars.sarrays     = new std::vector<std::vector<std::string *>*>();
  g_svars.barrays     = new std::vector<std::vector<std::vector<char>*>*>();
  g_svars.s_pills     = new std::vector<std::vector<std::string *>*>();
  g_svars.b_pills     = new std::vector<std::vector<std::vector<char>*>*>();
  g_svars.thrs        = new std::vector<std::thread*>();

  // TODO: allow replay_size bigger than MEBIBYTE
  uint64_t replay_size = ( per_sec_throughput * emit_dur );
  //if (replay_size > MEBIBYTE) {
  //  replay_size = MEBIBYTE;
  //}
  g_svars.replay_size = replay_size;
  g_svars.remaining_replay_size = replay_size * nr_threads;

  for (int i=0; i < nr_threads; ++i) {
    auto v = new std::vector<std::string *>();
    g_svars.sarrays->push_back(v);
  }

  for (int i=0; i < nr_threads; ++i) {
    auto v = new std::vector<std::vector<char>*>();
    g_svars.barrays->push_back(v);
  }

  for (int i=0; i < nr_threads; ++i) {
    auto v = new std::vector<std::string *>();
    g_svars.s_pills->push_back(v);
  }

  for (int i=0; i < nr_threads; ++i) {
    auto v = new std::vector<std::vector<char>*>();
    g_svars.b_pills->push_back(v);
  }
}

void runDGSender() {
  pthread_barrier_init(&barrier, NULL, g_svars.nr_threads);

  auto& s_arrs = *g_svars.sarrays;
  auto& b_arrs = *g_svars.barrays;
  auto& spills = *g_svars.s_pills;
  auto& bpills = *g_svars.b_pills;
  bool send_bytes = false;

  // Check size of the first array in s_arrs and b_arrs and
  // determine whether to drain g_svars.sarrays or g_svars.barrays
  if (s_arrs[0]->empty()) {
    assert((!b_arrs[0]->empty()) && "No tuple is loaded");
    send_bytes = true;
  }

  for (int i=0; i < g_svars.nr_threads; ++i) {
    std::thread* t;
    if (send_bytes) {
      t = new std::thread(mt_emitScheduler<std::vector<char>*>, i, &g_svars,
                          b_arrs.at(i), bpills.at(i));
    }
    else {
      t = new std::thread(mt_emitScheduler<std::string*>, i, &g_svars,
                          s_arrs.at(i), spills.at(i));
    }
    g_svars.thrs->push_back(t);
  }

  for (int i=0; i < g_svars.nr_threads; ++i) {
    g_svars.thrs->at(i)->join();
  }

  // delete allocations
  for (auto i = s_arrs.begin(); i != s_arrs.end(); ++i) {
    for (auto j = (*i)->begin(); j != (*i)->end(); ++j) { delete (*j); }
    delete (*i);
  }
  delete g_svars.sarrays;

  for (auto i = b_arrs.begin(); i != b_arrs.end(); ++i) {
    for (auto j = (*i)->begin(); j != (*i)->end(); ++j) { delete (*j); }
    delete (*i);
  }
  delete g_svars.barrays;

  for (auto i = spills.begin(); i != spills.end(); ++i) {
    for (auto j = (*i)->begin(); j != (*i)->end(); ++j) { delete (*j); }
    delete (*i);
  }
  delete g_svars.s_pills;

  for (auto i = bpills.begin(); i != bpills.end(); ++i) {
    for (auto j = (*i)->begin(); j != (*i)->end(); ++j) { delete (*j); }
    delete (*i);
  }
  delete g_svars.b_pills;

  for (auto i = g_svars.thrs->begin(); i != g_svars.thrs->end(); ++i) {
    delete (*i);
  }
  delete g_svars.thrs;
}

long loadOneTuple(char* s, size_t length) {

  assert((!g_svars.barrays->empty()) && "initDGSender() must be used before.");

  uint64_t nr_tuples = g_svars.replay_size * g_svars.nr_threads;

  if (g_svars.remaining_replay_size > 0) {

    auto jbytes = new std::vector<char>(s, s + length);
  // Tuple Duplication Debugging
  ////
  //uint64_t * tmp = reinterpret_cast<uint64_t *>(jbytes->data());
  //*tmp = (nr_tuples - g_svars.remaining_replay_size);
  ////
    g_svars.barrays->at(g_svars.thr_id)->push_back(jbytes);

    g_svars.remaining_replay_size--;
    if ((g_svars.replay_size - g_svars.iter) == 1) {
      g_svars.iter = 0;
      ++g_svars.thr_id;
    } else {
      ++g_svars.iter;
    }
  }

  return g_svars.remaining_replay_size;
}

void loadPoisonPills(char* s, size_t length) {

  assert((!g_svars.barrays->empty()) && "initDGSender() must be used before.");

  for (int i=0; i < g_svars.nr_threads; ++i) {
    auto jbytes = new std::vector<char>(s, s + length);
  // Tuple Duplication Debugging
  ////
  //uint64_t * tmp = reinterpret_cast<uint64_t *>(jbytes->data());
  //*tmp = -1;
  ////
    g_svars.b_pills->at(i)->push_back(jbytes);
  }
}

void loadAllStrTuples() {
  // create replay tuples for each thread
  for (int i=0; i < g_svars.nr_threads; ++i) {
    prepare_replays(g_svars.obj,
                    (i * g_svars.replay_size) + g_svars.tup_idx_offset,
                    g_svars.replay_size,
                    g_svars.sarrays->at(i));
  }
}

void prepare_replays(JavaAgentHandler* obj,
                     uint64_t offset, uint64_t size,
                     std::vector<std::string *>* v) {
  for (uint64_t i=0; i < size; ++i) {
    std::string ret_str = obj->get_tuple(offset + i);
    std::string* str = new std::string(ret_str);
    v->push_back(str);
  }
}

const char* recvStr(int64_t ch) {
//return st_recvStr(ch);
  return mt_recvStr(ch);
}

const char* fetchStr(int64_t ch) {
  return mt_fetchStr(ch);
}

const char* st_recvStr(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  zmq::message_t zmqmsg;
  t_vars->sock->recv(&zmqmsg);
  logTS(t_vars->ch_fpc, 0);

  return static_cast<const char *>(zmqmsg.data());
}

// Read next queue on a failed read
const char*  mt_recvStr(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  static uint64_t i = 0;
  auto vec_q = (*t_vars->queues);
  uint64_t nr_q = vec_q.size();
  char* str = nullptr;
  bool ret;

  if (1 == nr_q) {
    while ( !vec_q[0]->pop(str) ) ;
  }
  else if (2 == nr_q) {
    do {
      ret = vec_q[i]->pop(str);
      i = !(i);
    } while ( !ret ) ;
  }
  else if (2 < nr_q) {
    do {
      ret = vec_q[i]->pop(str);
      ++i;
      if (i == nr_q) i = 0;
    } while ( !ret ) ;
  }
  return str;
}

// deprecated
const char* mt_fetchStr(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  static uint64_t i = 0;
  auto vec_q = (*t_vars->queues);
  uint64_t nr_q = vec_q.size();
  char* str = nullptr;
  bool ret;

  if (1 == nr_q) {
    while ( !vec_q[0]->pop(str) ) ;
  }
  else if (2 == nr_q) {
    do {
      ret = vec_q[i]->pop(str);
      i = !(i);
    } while ( !ret ) ;
  }
  else if (2 < nr_q) {
    do {
      ret = vec_q[i]->pop(str);
      ++i;
      if (i == nr_q) i = 0;
    } while ( !ret ) ;
  }
  return str;
}

template <typename T>
void mt_emitScheduler(int32_t tid, struct sv_vars* svvs,
                      std::vector<T>* v, std::vector<T>* v2) {
  int64_t ch_fpc, ch_blk, ch_ovr, ch_bid;     // channels for the server
  auto& vec = *v;
  auto& vec2 = *v2;

  ch_fpc  = openChannel("SwigCPPDGServer      FPC_10MS", ASCII, NET_CONF);
  ch_blk  = openChannel("SwigCPPDGServer    BLKFPC10MS", ASCII, NET_CONF);
  ch_ovr  = openChannel("SwigCPPDGServer       OVERRUN", ASCII, NET_CONF);
  ch_bid  = openChannel("SwigCPPDGServer      IDENTITY", ASCII, NET_CONF);

  // Create a context and a socket
  zmq::context_t send_ctx (1);
  zmq::socket_t send_sock (send_ctx, ZMQ_PUSH);

  // Set socket uptions
  int val = TCP_BUF_SIZE;
  send_sock.setsockopt(ZMQ_SNDBUF, &val, sizeof(val));
  //val = VAL_ZMQ_IMMEDIATE;
  //send_sock.setsockopt(ZMQ_IMMEDIATE, &val, sizeof(val));

  // Start binding
  std::ostringstream ep_bind_url;
  ep_bind_url << "tcp://*:" << (svvs->sv_port_nr + tid);
  send_sock.bind(ep_bind_url.str());

  std::ostringstream log;
  log << "[tid: "<< tid << "] Entering into a mute state.." << "\n";
  std::cout << log.str();

  // Set pollout
//zmq::pollitem_t item;
//item.socket = static_cast<void*>(send_sock);
//item.fd     = 0;
//item.events = ZMQ_POLLOUT;

  // Send an initial socket
  const char * buf = "init";
  zmq::message_t zmqmsg (buf, strlen(buf) + 1);
  if (!send_sock.send(zmqmsg)) {
    std::ostringstream log;
    log << "[tid: " << tid << "] Error in ZMQ socket send (0)" << "\n";
    std::cerr << log.str();
  }

  pthread_barrier_wait(&barrier);
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  log.str("");
  log.clear();
  log << "[tid: " << tid << "] ";
  log << "Begin pushing to the downstream pipeline nodes." << "\n";
  std::cout << log.str();

  int64_t zmq_overhead    = POLL_ENQUEUE_OVERHEAD;
                                  // Estimated per-tuple ZMQ emission overhead
                                  // measured with st_testEmitUpperBound
  int64_t per_ms_throughput;      // Convert to tuples per millisecond
  if ((uint64_t)svvs->per_sec_throughput > MILLI_S_PER_S) {
    per_ms_throughput     = svvs->per_sec_throughput / MILLI_S_PER_S;
  }
  else {
    per_ms_throughput     = svvs->per_sec_throughput;
  }
  int64_t budget_per_emission;    // Calculate budget per emission
  budget_per_emission     = NANO_S_PER_S / svvs->per_sec_throughput;
  int64_t residue_per_emission;   // Duration for polling per emission
  residue_per_emission    = budget_per_emission - (zmq_overhead + svvs->slag);

  // On GCE:
  // getTS:      89 ns
  // logTS:   1,619 ns  (ID handler)
  // logTS:       3 ns  (NULL handler)
  // nologTS:     2 ns  (NULL handler)
  // rdtsc:      13 ns  (29 cycles)
  // rdtscp:     16 ns  (36 cycles)
  if (budget_per_emission < (zmq_overhead + svvs->slag)) {
    exit(EXIT_FAILURE);
  }

  auto begin = vec.begin();
  auto end   = vec.end();
  auto iter  = begin;

  timespec curTime, emitEnd, budgetEnd;
  const timespec TS_NULL    = { .tv_sec = 0, .tv_nsec = 0 };
  const timespec TS_RESIDUE = { .tv_sec = 0, .tv_nsec = residue_per_emission };
  const timespec TS_BUDGET  = { .tv_sec = 0, .tv_nsec = svvs->time_budget };

  timespec TS_START;
  pthread_barrier_wait(&barrier);
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  getTS(&TS_START);

  const timespec TS_END     = { .tv_sec  = TS_START.tv_sec + svvs->emit_dur,
                                .tv_nsec = TS_START.tv_nsec };

  addTS(&curTime, &TS_START, &TS_NULL);
  addTS(&budgetEnd, &TS_START, &TS_NULL);

  while (true) {
    for (int i=0; i < per_ms_throughput; ++i) {

      auto& str = *(*iter++);

    // Tuple Duplication Debugging
    ////
    //logTS(ch_fpc, 0L);
    //uint64_t idx = *(reinterpret_cast<uint64_t *>(&str[0]));
    //logTS(ch_bid, idx);
    ////

      send_sock.send(&str[0], str.size());
    //std::string* str = *iter++;
    //send_sock.send(str->c_str(), str->size());

      if (iter == end) { iter = begin; }

      // per-emit-polling
      addTS(&emitEnd, &curTime, &TS_RESIDUE);
      do {
        getTS(&curTime);
      } while(compareTS(&curTime, &emitEnd) < 0);

    }
    // Polling for re-synchronizing
    // Overhead of getTS determines granularity of the re-synchronization
    addTS(&budgetEnd, &budgetEnd, &TS_BUDGET);
    do {
      getTS(&curTime);
    } while(compareTS(&curTime, &budgetEnd) < 0);

    if (compareTS(&curTime, &TS_END) >= 0) {
      break;
    }
  }

  if (svvs->pill_dur > 0) {
    auto begin = vec2.begin();
    auto end   = vec2.end();
    auto iter  = begin;

    timespec TS_PP_START;
    getTS(&TS_PP_START);
    const timespec TS_END     = { .tv_sec  = TS_PP_START.tv_sec + svvs->pill_dur,
                                  .tv_nsec = TS_PP_START.tv_nsec };

    addTS(&curTime, &TS_PP_START, &TS_NULL);
    addTS(&budgetEnd, &TS_PP_START, &TS_NULL);

    while (true) {
      for (int i=0; i < per_ms_throughput; ++i) {

        auto& str = *(*iter++);
        send_sock.send(&str[0], str.size());
      //logTS(ch_fpc, 0L);
        if (iter == end) { iter = begin; }

        // per-emit-polling
        addTS(&emitEnd, &curTime, &TS_RESIDUE);
        do {
          getTS(&curTime);
        } while(compareTS(&curTime, &emitEnd) < 0);

      }
      // Polling for re-synchronizing
      // Overhead of getTS determines granularity of the re-synchronization
      addTS(&budgetEnd, &budgetEnd, &TS_BUDGET);
      do {
        getTS(&curTime);
      } while(compareTS(&curTime, &budgetEnd) < 0);

      if (compareTS(&curTime, &TS_END) >= 0) {
        break;
      }
    }
  }

  logTS(ch_fpc, -1);
  logTS(ch_ovr, -1);
  logTS(ch_blk, -1);
}

int64_t emit(JavaAgentHandler* obj, int64_t ch, int64_t emitDuration,
             int64_t emitThroughput, int64_t tupleIdx, int64_t tupleIdxOffset,
             int64_t timeBudget) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  // It takes 246.675725 nanoseconds to receive an empty string, and
  // it takes 889.87854 nanoseconds to receive a sample campaign
  int nr_iterations = 1000 * 1000;
  timespec start, end, diff, quotient;
  int blocked = 0;

  getTS(&start);
  for (int i=0; i<nr_iterations; ++i) {
    if (zmq::poll(t_vars->item, VAL_ZMQ_POLLITEMS, 0) > 0) {
      std::string str = obj->get_jstring();
      t_vars->sock->send(str.c_str(), str.length() + 1);
    } else {
      blocked++;
    }
  }
  getTS(&end);
  subtractTS(&diff, &start, &end);
  divideTS(&quotient, &diff, nr_iterations);
  char buf[1024];
  formatTS(&start, buf, 1024);
  std::cout << "Start:     " << buf << std::endl;
  formatTS(&end, buf, 1024);
  std::cout << "End:       " << buf << std::endl;
  formatDuration(&diff, buf, 1024, true);
  std::cout << "Duration:  " << buf << std::endl;
  std::cout << "Calls:     " << l2aCommas(nr_iterations, buf) << std::endl;
  formatDuration(&quotient, buf, 1024, true);
  std::cout << "time/call: " << buf << std::endl;
  std::cout << "blocked: " << blocked << std::endl;

  return tupleIdx;
}

void st_cleanup(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);
  if (nullptr != t_vars->item) {
    delete t_vars->item;
  }
  t_vars->sock->close();
  t_vars->ctx->close();
  delete t_vars;
}

void mt_cleanup(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);
  if (nullptr != t_vars->item) {
    delete t_vars->item;
  }
  t_vars->sock->close();
  t_vars->ctx->close();
  delete t_vars;
}

void st_run(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);
  if (t_vars->isServer) {
    t_vars->ch_fpc = openChannel("SwigCPPDGServer      FPC_10MS", ASCII, NET_CONF);

    // Create a context and a socket
    t_vars->ctx   = new zmq::context_t(1);
    t_vars->sock  = new zmq::socket_t(*t_vars->ctx, ZMQ_PUSH);

    // Set socket options
    int val = TCP_BUF_SIZE;
    t_vars->sock->setsockopt(ZMQ_SNDBUF, &val, sizeof(val));
    val = VAL_ZMQ_IMMEDIATE;
    t_vars->sock->setsockopt(ZMQ_IMMEDIATE, &val, sizeof(val));

    // Start binding
    std::ostringstream ep_bind_url;
    ep_bind_url << "tcp://*:" << t_vars->sv_port_nr;
    t_vars->sock->bind(ep_bind_url.str());
    std::cout << "Entering into a mute state.." << std::endl;

    // Set pollout
    t_vars->item         = new zmq::pollitem_t();
    t_vars->item->socket = static_cast<void*>(*t_vars->sock);
    t_vars->item->fd     = 0;
    t_vars->item->events = ZMQ_POLLOUT;

    // Send an initial message
    const char * buf = "init";
    zmq::message_t zmqmsg (buf, strlen(buf) + 1);
    if (!t_vars->sock->send(zmqmsg)) {
      std::cerr << "Error in ZMQ socket send" << std::endl;
    }

    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    std::cout << "Begin pushing to the downstream pipeline nodes." << "\n";

    //warmUp
  } else {
    t_vars->ch_fpc = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
        NET_CONF);

    // Create a context and a socket
    t_vars->ctx   = new zmq::context_t(1);
    t_vars->sock  = new zmq::socket_t(*t_vars->ctx, ZMQ_PULL);
    t_vars->item  = nullptr;

    // Establish a receive socket
    std::ostringstream ep_connect_url;
    auto addresses = (*t_vars->sv_addresses);
    ep_connect_url << "tcp://" << addresses[0];

    int val = TCP_BUF_SIZE;
    t_vars->sock->setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

    t_vars->sock->connect(ep_connect_url.str());

    // Read initial message
    zmq::message_t zmqmsg;
    if (!t_vars->sock->recv(&zmqmsg)) {
      std::cerr << "Error in ZMQ socket receive" << std::endl;
    }
  }
}

void mt_run(int64_t ch) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);
  auto addresses = (*t_vars->sv_addresses);

  if (t_vars->isServer) {
  } else {
    t_vars->queues = new std::vector<boost::lockfree::spsc_queue<char*,
      boost::lockfree::capacity<QUEUE_SIZE>>*>();

    for (int i=0; i < t_vars->nr_threads; ++i) {
      auto* queue = new boost::lockfree::spsc_queue<char*,
        boost::lockfree::capacity<QUEUE_SIZE>>();
      t_vars->queues->push_back(queue);
    }

    std::thread tids[t_vars->nr_threads];
    for (int i=0; i < t_vars->nr_threads; ++i) {
      tids[i] = std::thread(mt_client, ch, i, addresses[i]);
    }

    for (int i=0; i < t_vars->nr_threads; ++i) {
      tids[i].detach();
    }
  }
}

void mt_client(int64_t ch, int32_t tid, std::string address) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  int64_t ch_cli = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
      NET_CONF);
  int64_t ch_fl  = openChannel("SwigCPPDGClient     FIRSTLAST", ASCII,
      NET_CONF);
  int64_t ch_fox = openChannel("SwigCPPDGClient      FOXSTART", ASCII,
      NET_CONF);

  auto  vec_q = (*t_vars->queues);
  auto* q     = (vec_q[tid]);
  char* str;

  // Establish a receive socket
  zmq::context_t recv_ctx (1);
  zmq::socket_t recv_sock (recv_ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << address;

  int val = 12582912;
  recv_sock.setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

  recv_sock.connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  recv_sock.recv(&zmqmsg);

  logTS(ch_fox, 1L);

  while (true) {
    zmq::message_t zmqmsg;
    recv_sock.recv(&zmqmsg);
    int msg_size = zmqmsg.size();
    str = new char[msg_size + 1]; // it includes the null terminator
    std::strncpy(str, static_cast<char*>(zmqmsg.data()), msg_size);
    str[msg_size] = '\0';
  //str = new std::string(static_cast<char*>(zmqmsg.data()));
    while ( !q->push(str) ) ;
    logTS(ch_fl, 0);
    logTS(ch_cli, 0);
  }
}

int64_t openSelfFeedChannel(int64_t emit_dur, int64_t per_sec_throughput,
    int64_t slag, int64_t nr_threads) {

  tcp_vars* t_vars = new tcp_vars();
  int64_t ch = reinterpret_cast<intptr_t>(t_vars);

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  t_vars->nr_threads    = nr_threads;
  t_vars->queues        = new std::vector<boost::lockfree::spsc_queue<char*,
    boost::lockfree::capacity<QUEUE_SIZE>>*>();

  for (int i=0; i < t_vars->nr_threads; ++i) {
    auto* queue = new boost::lockfree::spsc_queue<char*,
      boost::lockfree::capacity<QUEUE_SIZE>>();
    t_vars->queues->push_back(queue);
  }

  sv_vars* vars = new sv_vars();
  vars->emit_dur           = emit_dur;
  vars->per_sec_throughput = per_sec_throughput;
  vars->slag               = slag;
  vars->nr_threads         = nr_threads;
  vars->time_budget        = TIME_BUDGET;

  std::thread tids[t_vars->nr_threads];

  for (int i=0; i < t_vars->nr_threads; ++i) {
    tids[i] = std::thread(mt_selfFeeder, ch, i, vars);
  }

  for (int i=0; i < t_vars->nr_threads; ++i) {
    tids[i].detach();
  }

  return ch;
}

void mt_selfFeeder(int64_t ch, int32_t tid, struct sv_vars* svvs) {
  tcp_vars* t_vars = reinterpret_cast<tcp_vars *>(ch);

  int64_t ch_cli = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
      NET_CONF);

  auto  vec_q = (*t_vars->queues);
  auto* q     = (vec_q[tid]);
  char* str;

  std::ostringstream log;
  log << "[tid: "<< tid << "] Entering into a mute state.." << "\n";
  std::cout << log.str();


  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  log.str("");
  log.clear();
  log << "[tid: " << tid << "] ";
  log << "Begin feeding to the spsc queues." << "\n";
  std::cout << log.str();

  int64_t zmq_overhead    = 0;    // No zmq overhead for selfFeeder
  int64_t per_ms_throughput;      // Convert to tuples per millisecond
  per_ms_throughput       = svvs->per_sec_throughput / MILLI_S_PER_S;
  int64_t budget_per_emission;    // Calculate budget per emission
  budget_per_emission     = NANO_S_PER_S / svvs->per_sec_throughput;
  int64_t residue_per_emission;   // Duration for polling per emission
  residue_per_emission    = budget_per_emission - (zmq_overhead + svvs->slag);

  // On GCE:
  // getTS:      89 ns
  // logTS:   1,619 ns  (ID handler)
  // logTS:       3 ns  (NULL handler)
  // nologTS:     2 ns  (NULL handler)
  // rdtsc:      13 ns  (29 cycles)
  // rdtscp:     16 ns  (36 cycles)
  if (budget_per_emission < (zmq_overhead + svvs->slag)) {
    exit(EXIT_FAILURE);
  }

  std::ostringstream sample;
  sample << "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\","
         << " \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\","
         << " \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\","
         << " \"ad_type\": \"mail\","
         << " \"event_type\": \"view\","
         << " \"event_time\": \"1534761527997\","
         << " \"tuple_idx\": \"1\","
         << " \"ip_address\": \"10.140.0.121\"}";

  int   temp_len = sample.str().size();
  char* temp_str = new char[temp_len + 1];
  std::strncpy(temp_str, sample.str().c_str(), temp_len);
  temp_str[temp_len] = '\0';

  timespec curTime, emitEnd, budgetEnd;
  const timespec TS_NULL    = { .tv_sec = 0, .tv_nsec = 0 };
  const timespec TS_RESIDUE = { .tv_sec = 0, .tv_nsec = residue_per_emission };
  const timespec TS_BUDGET  = { .tv_sec = 0, .tv_nsec = svvs->time_budget };

  timespec TS_START;
  getTS(&TS_START);

  const timespec TS_END     = { .tv_sec  = TS_START.tv_sec + svvs->emit_dur,
                                .tv_nsec = TS_START.tv_nsec };

  addTS(&curTime, &TS_START, &TS_NULL);
  addTS(&budgetEnd, &TS_START, &TS_NULL);

  while (true) {
    for (int i=0; i < per_ms_throughput; ++i) {

      str = new char[temp_len + 1]; // it includes the null terminator
      std::strncpy(str, temp_str, temp_len);
      str[temp_len] = '\0';
      while ( !q->push(str) ) ;
      logTS(ch_cli, 0);

      // per-emit-polling
      addTS(&emitEnd, &curTime, &TS_RESIDUE);
      do {
        getTS(&curTime);
      } while(compareTS(&curTime, &emitEnd) < 0);

    }
    // Polling for re-synchronizing
    // Overhead of getTS determines granularity of the re-synchronization
    addTS(&budgetEnd, &budgetEnd, &TS_BUDGET);
    do {
      getTS(&curTime);
    } while(compareTS(&curTime, &budgetEnd) < 0);

    if (compareTS(&curTime, &TS_END) >= 0) {
      break;
    }
  }

  delete temp_str;

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  log.str("");
  log.clear();
  log << "[tid: " << tid << "] ";
  log << "Finished feeding to the spsc queues." << "\n";
  std::cout << log.str();
}

struct fetching_test_vars {
  int64_t ch_fpc;
};

int64_t prepareFetchingTest() {
  fetching_test_vars* ft_vars = new fetching_test_vars();
  int64_t ch = reinterpret_cast<intptr_t>(ft_vars);
  ft_vars->ch_fpc = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
                                NET_CONF);
  return ch;
}

char* fetchCStr() {
  static char temp_str[] =
    "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\","
    " \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\","
    " \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\","
    " \"ad_type\": \"mail\","
    " \"event_type\": \"view\","
    " \"event_time\": \"1534761527997\","
    " \"tuple_idx\": \"1\","
    " \"ip_address\": \"10.140.0.121\"}";

  return temp_str;
}

void fetchCStrBucket(char** CStrBucket) {
  // Refer to ../cloud_profiler.i for actual implementation
  return;
}

int64_t fetchInt(int64_t ch) {
  fetching_test_vars* ft_vars = reinterpret_cast<fetching_test_vars *>(ch);

  static int64_t temp_int = 0L;

  logTS(ft_vars->ch_fpc, temp_int);

  return temp_int++;
}

int64_t openZMQChannel(std::string address) {
  zmq_vars* z_vars = new zmq_vars();
  int64_t ch = reinterpret_cast<intptr_t>(z_vars);

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  z_vars->ch_fpc = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
                               NET_CONF);

  z_vars->tm_vars = nullptr;

  z_vars->sv_address = new std::string(address);
  z_vars->ctx = new zmq::context_t();
  z_vars->sock = new zmq::socket_t(*z_vars->ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << *z_vars->sv_address;

  int val = TCP_BUF_SIZE;
  z_vars->sock->setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

  z_vars->sock->connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  z_vars->sock->recv(&zmqmsg);

  return ch;
}

timer_vars* tm_vars = nullptr;
timeout_vars* tmo_vars = nullptr;
void timer_fn(struct timer_vars* tv);
void timer_opt_fn(struct timer_vars* tv, int64_t start_sec, int64_t start_ns);

int64_t openZMQBucketChannel(std::string address) {
  zmq_vars* z_vars = new zmq_vars();
  int64_t ch = reinterpret_cast<intptr_t>(z_vars);

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  if (tm_vars == nullptr) {
    tm_vars = new timer_vars();
    tm_vars->timer_thr = new std::thread(timer_fn, tm_vars);
    tm_vars->timer_thr->detach();
  }
  z_vars->tm_vars = tm_vars;

  z_vars->sv_address = new std::string(address);
  z_vars->ctx = new zmq::context_t();
  z_vars->sock = new zmq::socket_t(*z_vars->ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << *z_vars->sv_address;

  int val = TCP_BUF_SIZE;
  z_vars->sock->setsockopt(ZMQ_RCVBUF, &val, sizeof(val));
  val = RECV_TIMEOUT;
  z_vars->sock->setsockopt(ZMQ_RCVTIMEO, &val, sizeof(val));

  z_vars->sock->connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  while (!z_vars->sock->recv(&zmqmsg)) ;

  return ch;
}

int64_t openZMQBktOptChannel(std::string address) {
  zmq_vars* z_vars = new zmq_vars();
  int64_t ch = reinterpret_cast<intptr_t>(z_vars);

  std::ostringstream bi;
  build_info(&bi);
  std::cout << bi.str();

  z_vars->ch_fpc = openChannel("SwigCPPDGClient      FPC_10MS", ASCII,
      NET_CONF);
  z_vars->ch_bid = openChannel("SwigCPPDGClient      IDENTITY", ASCII,
      NET_CONF);
  z_vars->sv_address = new std::string(address);
  z_vars->ctx = new zmq::context_t();
  z_vars->sock = new zmq::socket_t(*z_vars->ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << *z_vars->sv_address;

  int val = TCP_BUF_SIZE;
  z_vars->sock->setsockopt(ZMQ_RCVBUF, &val, sizeof(val));
  val = RECV_TIMEOUT;
  z_vars->sock->setsockopt(ZMQ_RCVTIMEO, &val, sizeof(val));

  z_vars->sock->connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  while (!z_vars->sock->recv(&zmqmsg)) ;

  return ch;
}

void initTimeoutVars(int64_t ch, int64_t nr_threads) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  timer_vars* tm;
  timeout_vars* tmo;

  if ((nullptr == tm_vars) && (nullptr == tmo_vars)) {
    uint64_t nr_bytes = sizeof(timeout_vars) * nr_threads;
    tmo = static_cast<timeout_vars *>(aligned_alloc(CACHELINE_SIZE, nr_bytes));
    if (nullptr == tmo) {
      std::cerr << "aligned_malloc of timer_vars failed" << "\n";
      exit(EXIT_FAILURE);
    }

    tm = new timer_vars();

    for (int i = 0; i < nr_threads; ++i) {
      tmo[i].next_timeout = 1;
    }

    timespec now;
    getTS(&now);

    tm->elapsed_intervals = 0;
    tm->timer_thr = new std::thread(timer_opt_fn, tm, now.tv_sec, now.tv_nsec);
    tm->timer_thr->detach();

    tm_vars = tm;
    tmo_vars = tmo;
  }
  z_vars->tm_vars = tm_vars;
  z_vars->tmo_vars = tmo_vars;
}

// An internal function only to be used in cloud_profiler.i
timeout_vars* getTimeoutVars(int64_t ch, int tid) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  return z_vars->tmo_vars + tid;
}

void closeZMQChannel(int64_t ch) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  timer_vars* tm = z_vars->tm_vars;

  // wait for the timer_thread to be finished
  if (tm != nullptr) {
    tm->terminate = true;
    while (!tm->terminated) ;
  }
  delete tm->timer_thr;
  delete tm;

  if (z_vars->sock->connected()) z_vars->sock->disconnect(*z_vars->sv_address);
  z_vars->sock->close();
  // z_vars->ctx->shutdown(); // available in the future version
  z_vars->ctx->close();

  closeChannel(z_vars->ch_fpc);
  delete z_vars->sock;
  delete z_vars->ctx;
  delete z_vars->sv_address;
  delete z_vars;
}


char* fetchZMQStr(int64_t ch) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  zmq::message_t zmqmsg;
  z_vars->sock->recv(&zmqmsg);
  logTS(z_vars->ch_fpc, 0L);
  return static_cast<char*>(zmqmsg.data());
}

// should be retired
char* fetchZMQStrNB(int64_t ch) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  zmq::message_t zmqmsg;
  if (0 == z_vars->sock->recv(&zmqmsg)) {
    return nullptr;
  }
  return static_cast<char*>(zmqmsg.data());
}

void* fetchZMQStrNB2(int64_t ch, zmq::message_t* msg) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  if (0 == z_vars->sock->recv(msg)) {
    return nullptr;
  }
// Tuple Duplication Debugging
////
//uint64_t idx = *(reinterpret_cast<uint64_t *>(msg->data()));
//logTS(z_vars->ch_bid, idx);
  logTS(z_vars->ch_fpc, 0L);
////
  return msg->data();
}

void* fetchZMQStrNB3(int64_t ch, int64_t msg) {
  return fetchZMQStrNB2(ch, reinterpret_cast<zmq::message_t*>(msg));
}

int fetchZMQBucket(int64_t chBucket, int tuningKnob, char** ZMQBucket) {
  // Refer to ../cloud_profiler.i for actual implementation
  return 0;
}

int fetchZMQBktOpt(int64_t chBucket, int tuningKnob, int tid, char** ZBktOpt) {
  // Refer to ../cloud_profiler.i for actual implementation
  return 0;
}

int fetchObjBktOpt(int64_t chBucket, int tuningKnob, int tid, char** byte2D) {
  // Refer to ../byte2d.i for actual implementation
  return 0;
}

void timer_fn(struct timer_vars* tm) {
  const timespec TS_SLEEP = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1 ms
  tm->terminated = false;

  while (!tm->terminate) {
  pselect(0, NULL, NULL, NULL, &TS_SLEEP, NULL);
  timespec now;
  getTS(&now);
  tm->elapsed_intervals = (uint64_t)((now.tv_sec * NANO_S_PER_S) + now.tv_nsec);
  }

  tm->terminated = true;
}

void timer_opt_fn(struct timer_vars* tm, int64_t start_sec, int64_t start_ns) {
  const timespec TS_NULL    = { .tv_sec = 0, .tv_nsec = 0 };
//const timespec TS_SLEEP   = { .tv_sec = 0, .tv_nsec = 10000 };   // 0.01 ms
  const timespec TS_BUDGET  = { .tv_sec = 0, .tv_nsec = BKT_TIMER_BUDGET };
  timespec TS_START = { .tv_sec = start_sec, .tv_nsec = start_ns };

  tm->terminated = false;

  timespec now, budgetEnd;

  addTS(&now, &TS_START, &TS_NULL);
  addTS(&budgetEnd, &TS_START, &TS_BUDGET);

  while (!tm->terminate) {
    do {
    //pselect(0, NULL, NULL, NULL, &TS_SLEEP, NULL);
      getTS(&now);
    } while(compareTS(&now, &budgetEnd) < 0);

    tm->elapsed_intervals += 1;
    addTS(&budgetEnd, &budgetEnd, &TS_BUDGET);
  }

  tm->terminated = true;
}

uint64_t elapsedIntervals(int64_t ch) {
  zmq_vars* z_vars = reinterpret_cast<zmq_vars *>(ch);
  timer_vars* tm = z_vars->tm_vars;
  if (tm != nullptr) {
    return tm->elapsed_intervals;
  }
  else {
    return 0UL;
  }
}
