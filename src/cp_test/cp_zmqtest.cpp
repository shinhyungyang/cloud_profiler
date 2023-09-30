#include "time_tsc.h"

#include "cloud_profiler.h"
#include "globals.h"
extern "C" {
#include "util.h"
#include "net.h"
}

#include "cxxopts.hpp"

#include <errno.h>        // for using nanosleep

//#include <chrono>       // for using std::this_thread::sleep_for
#include <thread>

//#include <sys/select.h> // for using select

#include <zmq.hpp>

#include <vector>
#include <boost/lockfree/spsc_queue.hpp>

#define MEBIBYTE              (1024 * 1024)
#define QUEUE_SIZE            (500 * 1000)
#define POLL_ENQUEUE_OVERHEAD (1900)
#define TCP_BUF_SIZE          (MEBIBYTE * 12)
#define VAL_ZMQ_POLLITEMS     (1)
#define VAL_ZMQ_IMMEDIATE     (1)

// Single-threaded implementation
void st_run();
  void st_testEmitUpperBound(zmq::socket_t & send_sock, zmq::pollitem_t & item);
  void      st_emitScheduler(zmq::socket_t & send_sock, zmq::pollitem_t & item);
  void               st_emit(zmq::socket_t & send_sock, zmq::pollitem_t & item);
  void             st_client();

// Multi-threaded implementation
void mt_run();
  void mt_emitScheduler(int32_t tid, int32_t port_nr);
  void        mt_client(int32_t tid, int32_t port_nr,
                        std::vector<boost::lockfree::spsc_queue<std::string*,
                                    boost::lockfree::capacity<QUEUE_SIZE>>*>* queues);
  void     mt_spsc_sink(std::vector<boost::lockfree::spsc_queue<std::string*,
                                    boost::lockfree::capacity<QUEUE_SIZE>>*>* queues);
  void     mt_sock_sink(std::vector<zmq::socket_t*>* sockets);
  void mt_client_socket(int32_t port_nr, zmq::context_t** c, zmq::socket_t** s);

std::string sv_dotted_quad;                 // server ip address
int32_t     sv_port_nr;                     // server port number
bool        isServer          = false;      // run as a server or a client
bool        useQueue          = true;       // use SPSC queue by default

int64_t emitDuration          = 35;         // seconds
int64_t perSecThroughput      = 32000;      // tuples per second
int64_t tupleIdx              = 0;
int64_t tupleIdxOffset        = 1000000000; // tuple offset for multiple DGs
int64_t timeBudget            = 1000000;    // one millisecond
int64_t recordDuration        = 35;         // record duration
int64_t tupleAggregationRatio = 8;          // number of sleeps per timeBudget
int64_t slag                  = 200;        // extra duration for wall-clock
                                            // re-synchronization (nanoseconds)
int64_t nr_threads            = 1;          // number of threads


const char* sample = "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\", \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\", \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\", \"ad_type\": \"mail\", \"event_type\": \"view\", \"event_time\": \"1534761527997\", \"tuple_idx\": \"1\", \"ip_address\": \"10.140.0.121\"}";
//const char* poisonPill = "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\", \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\", \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\", \"ad_type\": \"mail\", \"event_type\": \"view\", \"event_time\": \"1534761527997\", \"tuple_idx\": \"-1\", \"ip_address\": \"10.140.0.121\"}";

void mt_emitScheduler(int32_t tid, int32_t port_nr) {
  int64_t ch_fpc, ch_blk, ch_ovr;             // channels for the server

  ch_fpc  = openChannel("CPPDGServer          FPC_10MS", ASCII, NET_CONF);
  ch_blk  = openChannel("CPPDGServer        BLKFPC10MS", ASCII, NET_CONF);
  ch_ovr  = openChannel("CPPDGServer           OVERRUN", ASCII, NET_CONF);

  // Create a context and a socket
  zmq::context_t send_ctx (1);
  zmq::socket_t send_sock (send_ctx, ZMQ_PUSH);

  // Set socket uptions
  int val = TCP_BUF_SIZE;
  send_sock.setsockopt(ZMQ_SNDBUF, &val, sizeof(val));
  val = VAL_ZMQ_IMMEDIATE;
  send_sock.setsockopt(ZMQ_IMMEDIATE, &val, sizeof(val));

  // Start binding
  std::ostringstream ep_bind_url;
  ep_bind_url << "tcp://*:" << port_nr;
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

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  log.str("");
  log.clear();
  log << "[tid: " << tid << "] ";
  log << "Begin pushing to the downstream pipeline nodes." << "\n";
  std::cout << log.str();

  int64_t zmqOverhead   = POLL_ENQUEUE_OVERHEAD;
                                  // Estimated per-tuple ZMQ emission overhead
                                  // measured with st_testEmitUpperBound
  int64_t perMSThroughput;        // Convert to tuples per millisecond
  perMSThroughput     = perSecThroughput / MILLI_S_PER_S;
  int64_t budgetPerEmission;      // Calculate budget per emission
  budgetPerEmission   = NANO_S_PER_S / perSecThroughput;
  int64_t residuePerEmission;     // Duration for polling per emission
  residuePerEmission  = budgetPerEmission - (zmqOverhead + slag);

  // On GCE:
  // getTS:      89 ns
  // logTS:   1,619 ns  (ID handler)
  // logTS:       3 ns  (NULL handler)
  // nologTS:     2 ns  (NULL handler)
  // rdtsc:      13 ns  (29 cycles)
  // rdtscp:     16 ns  (36 cycles)
  if (budgetPerEmission < (zmqOverhead + slag)) {
    exit(EXIT_FAILURE);
  }

  timespec curTime, emitEnd, budgetEnd;
  const timespec TS_NULL    = { .tv_sec = 0, .tv_nsec = 0 };
  const timespec TS_RESIDUE = { .tv_sec = 0, .tv_nsec = residuePerEmission };
  const timespec TS_BUDGET  = { .tv_sec = 0, .tv_nsec = timeBudget };

  timespec TS_START;
  getTS(&TS_START);

  const timespec TS_END     = { .tv_sec  = TS_START.tv_sec + emitDuration,
                                .tv_nsec = TS_START.tv_nsec };

  addTS(&curTime, &TS_START, &TS_NULL);
  addTS(&budgetEnd, &TS_START, &TS_NULL);

  while (true) {
    for (int i=0; i < perMSThroughput; ++i) {
      send_sock.send(sample, strlen(sample) + 1); // emit
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

  logTS(ch_fpc, -1);
  logTS(ch_ovr, -1);
  logTS(ch_blk, -1);
}

void st_testEmitUpperBound(zmq::socket_t & send_sock, zmq::pollitem_t & item) {
  int64_t ch_fpc, ch_blk, ch_ovr;             // channels for the server

  ch_fpc  = openChannel("CPPDGServer          FPC_10MS", ASCII, NET_CONF);
  ch_blk  = openChannel("CPPDGServer        BLKFPC10MS", ASCII, NET_CONF);
  ch_ovr  = openChannel("CPPDGServer           OVERRUN", ASCII, NET_CONF);

  for (int i=0; i < perSecThroughput; ++i) {
    if (zmq::poll(&item, VAL_ZMQ_POLLITEMS, 0) > 0) {
      logTS(ch_fpc, tupleIdx + tupleIdxOffset);
      if (!send_sock.send(sample, strlen(sample) + 1)) {
        std::cerr << "Error in ZMQ socket send (2)" << std::endl;
      }
    } else {
      logTS(ch_blk, tupleIdx + tupleIdxOffset);
    }
  }

  logTS(ch_fpc, -1);
  logTS(ch_ovr, -1);
  logTS(ch_blk, -1);
}

void st_emitScheduler(zmq::socket_t & send_sock, zmq::pollitem_t & item) {
  int64_t ch_fpc, ch_blk, ch_ovr;             // channels for the server

  ch_fpc  = openChannel("CPPDGServer          FPC_10MS", ASCII, NET_CONF);
  ch_blk  = openChannel("CPPDGServer        BLKFPC10MS", ASCII, NET_CONF);
  ch_ovr  = openChannel("CPPDGServer           OVERRUN", ASCII, NET_CONF);

  int64_t zmqOverhead   = POLL_ENQUEUE_OVERHEAD;
                                  // Estimated per-tuple ZMQ emission overhead
                                  // measured with st_testEmitUpperBound
  int64_t perMSThroughput;        // Convert to tuples per millisecond
  perMSThroughput     = perSecThroughput / MILLI_S_PER_S;
  int64_t budgetPerEmission;      // Calculate budget per emission
  budgetPerEmission   = NANO_S_PER_S / perSecThroughput;
  int64_t residuePerEmission;     // Duration for polling per emission
  residuePerEmission  = budgetPerEmission - (zmqOverhead + slag);

  // On GCE:
  // getTS:      89 ns
  // logTS:   1,619 ns  (ID handler)
  // logTS:       3 ns  (NULL handler)
  // nologTS:     2 ns  (NULL handler)
  // rdtsc:      13 ns  (29 cycles)
  // rdtscp:     16 ns  (36 cycles)
  if (budgetPerEmission < (zmqOverhead + slag)) {
    exit(EXIT_FAILURE);
  }

  timespec curTime, emitEnd, budgetEnd;
  const timespec TS_NULL    = { .tv_sec = 0, .tv_nsec = 0 };
  const timespec TS_RESIDUE = { .tv_sec = 0, .tv_nsec = residuePerEmission };
  const timespec TS_BUDGET  = { .tv_sec = 0, .tv_nsec = timeBudget };

  timespec TS_START;
  getTS(&TS_START);

  const timespec TS_END     = { .tv_sec  = TS_START.tv_sec + emitDuration,
                                .tv_nsec = TS_START.tv_nsec };

  addTS(&curTime, &TS_START, &TS_NULL);
  addTS(&budgetEnd, &TS_START, &TS_NULL);

  while (true) {
    for (int i=0; i < perMSThroughput; ++i) {
      send_sock.send(sample, strlen(sample) + 1); // emit
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

  logTS(ch_fpc, -1);
  logTS(ch_ovr, -1);
  logTS(ch_blk, -1);
}

void st_emit(zmq::socket_t & send_sock, zmq::pollitem_t & item) {
  int64_t ch_fpc, ch_blk, ch_ovr;             // channels for the server

  ch_fpc  = openChannel("CPPDGServer          FPC_10MS", ASCII, NET_CONF);
  ch_blk  = openChannel("CPPDGServer        BLKFPC10MS", ASCII, NET_CONF);
  ch_ovr  = openChannel("CPPDGServer           OVERRUN", ASCII, NET_CONF);

  timespec startTime, endTime, curTime, emitEnd;

  int64_t perMSThroughput;          // convert to tuples per millisecond
  perMSThroughput = perSecThroughput / MILLI_S_PER_S;

  timespec ts_emitDuration;         // convert emitDuration to timespec type
  ts_emitDuration.tv_sec = emitDuration;
  ts_emitDuration.tv_nsec = 0;
  
  timespec ts_timeBudget;           // convert timeBudget to timespec type
  ts_timeBudget.tv_sec = 0;
  ts_timeBudget.tv_nsec = timeBudget;

  tupleIdx = 0L;

  getTS(&startTime);
  addTS(&endTime, &startTime, &ts_emitDuration);
  addTS(&emitEnd, &startTime, &ts_timeBudget);

  while (true) {
    for (int i=0; i < perMSThroughput; ++i) {
      getTS(&curTime);
      if (compareTS(&curTime, &emitEnd) >= 0) {
        for (int j=0; j < (perMSThroughput - i); ++j) {
          logTS(ch_blk, tupleIdx + tupleIdxOffset);
          tupleIdx++;
        }
        logTS(ch_ovr, tupleIdx + tupleIdxOffset);
        break;
      }

      if (zmq::poll(&item, VAL_ZMQ_POLLITEMS, 0) > 0) {
        logTS(ch_fpc, tupleIdx + tupleIdxOffset);
        if (!send_sock.send(sample, strlen(sample) + 1)) {
          std::cerr << "Error in ZMQ socket send (2)" << std::endl;
        }
      } else {
        logTS(ch_blk, tupleIdx + tupleIdxOffset);
      }

    //===============================
    //Different sleep implementations
    //===============================
    //1. C++
    //std::this_thread::sleep_for(std::chrono::nanoseconds(PER_TUPLE_SLEEP));
    //2. select
    //select(0, NULL, NULL, NULL, &tv);
    //tv.tv_usec = PER_TUPLE_SLEEP / 1000; // convert to microseconds
    //tv.tv_sec = 0;
    //3. POSIX nanosleep
    //int res;
    //do {
    //  res = nanosleep(&ts, &ts);
    //} while (res && errno == EINTR);
    //ts.tv_sec = 0;
    //ts.tv_nsec = PER_TUPLE_SLEEP;
    //4. POSIX pselect
    //   It may be similar to select, because we are not using the sigmask.
    //   This version checks if the kernel interrupted during the blocking.
    //do {
    //  pselect(0, NULL, NULL, NULL, &ts, NULL);
    //} while (res < 0 && errno == EINTR);
    //ts.tv_sec = 0;
    //ts.tv_nsec = adjusted_unitSleep;
      tupleIdx++;
    }

    getTS(&curTime);
    if (compareTS(&curTime, &emitEnd) >= 0) {
      addTS(&emitEnd, &curTime, &ts_timeBudget);
    } else {
      addTS(&emitEnd, &emitEnd, &ts_timeBudget);
    }

    if (compareTS(&curTime, &endTime) >= 0) {
      break;
    }
  }

//while (true) {
    logTS(ch_fpc, -1);
//  send_sock.send(poisonPill, strlen(poisonPill) + 1);
    logTS(ch_ovr, -1);
    logTS(ch_blk, -1);
//}
}

int main (int argc, char ** argv) {

  //
  // parse command-line options:
  cxxopts::Options options("zmq_test",
                           "cloud_profiler zmq test");
  options.add_options()
  ("sv_ip",               "DG IP address",
   cxxopts::value<std::string>())
  ("sv_p",                "DG Port number",
   cxxopts::value<int32_t>(sv_port_nr))
  ("d,duration",          "emit duration (seconds)",
   cxxopts::value<int64_t>(emitDuration))
  ("t,throughput",        "emit throughput (tuples per second)",
   cxxopts::value<int64_t>(perSecThroughput))
  ("o,tuple_offset",      "tuple index offset",
   cxxopts::value<int64_t>(tupleIdxOffset))
  ("r,record_duration",   "record duration (seconds)",
   cxxopts::value<int64_t>(recordDuration))
  ("a,tuple_aggregation", "number of tuples aggregated per emission",
   cxxopts::value<int64_t>(tupleAggregationRatio))
  ("s,server",            "Run as a server",
   cxxopts::value<bool>(isServer))
  ("noqueue",            "Do not use SPSC queues",
   cxxopts::value<bool>())
  ("l,slag",              "Extra duration for re-synchronizing to the wall-clock",
   cxxopts::value<int64_t>(slag))
  ("threads",             "Number of threads",
   cxxopts::value<int64_t>(nr_threads))
  ("h,help",              "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up data to request configuration:

  if (options.count("sv_ip")) {
    sv_dotted_quad = options["sv_ip"].as<std::string>();
  } else {
    sv_dotted_quad = "127.0.0.1";
  }
  if (!options.count("sv_p")) {
    sv_port_nr = 9101;
  }
  if (options.count("noqueue")) {
    useQueue = false;
  }

//st_run();
  mt_run();

  return 0;
}

//int main() {
///***************************************************************************
// * Tuple Aggregation Ratio Algorithm
// * E.g.,
// * Tuple Aggregation Ratio: 0.125   [0, 10]
// * Required Throughput:     32 T/ms (32 KT/s)
// * Time budget:              1 ms   (10^6 ns)
// * Unit Burst:       (32/8=) 4 tuples per unit
// * Unit Sleep:
// * ************************************************************************/
//int UNIT_BURST          = (perSecThroughput / tupleAggregationRatio);
//int UNIT_SLEEP          = (
//int PER_TUPLE_SLEEP     = (timeBudget / perSecThroughput); // in nanoseconds
//int UNIT_SLEEP          = (timeBudget / tupleAggregationRatio); // in nanoseconds
//int NUM_BURST           = perSecThroughput / tupleAggregationRatio;
//int adjusted_unitSleep  = UNIT_SLEEP;
//}

void st_run() {
  if (isServer) {
    // Create a context and a socket
    zmq::context_t send_ctx (1);
    zmq::socket_t send_sock (send_ctx, ZMQ_PUSH);

    // Set socket uptions
    int val = TCP_BUF_SIZE;
    send_sock.setsockopt(ZMQ_SNDBUF, &val, sizeof(val));
    val = VAL_ZMQ_IMMEDIATE;
    send_sock.setsockopt(ZMQ_IMMEDIATE, &val, sizeof(val));

    // Start binding
    std::ostringstream ep_bind_url;
    ep_bind_url << "tcp://*:" << sv_port_nr;
    send_sock.bind(ep_bind_url.str());
    std::cout << "Entering into a mute state.." << std::endl;

    // Set pollout
    zmq::pollitem_t item;
    item.socket = static_cast<void*>(send_sock);
    item.fd     = 0;
    item.events = ZMQ_POLLOUT;

    // Send an initial socket
    const char * buf = "init";
    zmq::message_t zmqmsg (buf, strlen(buf) + 1);
    if (!send_sock.send(zmqmsg)) {
      std::cerr << "Error in ZMQ socket send (0)" << std::endl;
    }

    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    std::cout << "Begin pushing to the downstream pipeline nodes." << "\n";

    //warmUp()


  //st_testEmitUpperBound(send_sock, item);
    st_emitScheduler(send_sock, item);
  //st_emit(send_sock, item);

  } else {
    st_client();
  }
}

void mt_run() {
  if (isServer) {
    std::thread tids[nr_threads];

    for (int i=0; i < nr_threads; ++i) {
      tids[i] = std::thread(mt_emitScheduler, i, sv_port_nr + i);
    }

    for (int i=0; i < nr_threads; ++i) {
      tids[i].join();
    }
  } else {
    if (useQueue) {
      std::vector<boost::lockfree::spsc_queue<std::string*,
        boost::lockfree::capacity<QUEUE_SIZE>>*> queues;

      for (int i=0; i < nr_threads; ++i) {
        auto* queue = new boost::lockfree::spsc_queue<std::string*,
          boost::lockfree::capacity<QUEUE_SIZE>>();
        queues.push_back(queue);
      }

      std::thread sink_tid = std::thread(mt_spsc_sink, &queues);

      std::thread tids[nr_threads];

      for (int i=0; i < nr_threads; ++i) {
        tids[i] = std::thread(mt_client, i, sv_port_nr + i, &queues);
      }

      for (int i=0; i < nr_threads; ++i) {
        tids[i].join();
      }

      sink_tid.join();
    } else {
      zmq::context_t* recv_ctx[nr_threads];
      zmq::socket_t* recv_sock[nr_threads];
      std::vector<zmq::socket_t*> sockets;

      for (int i=0; i < nr_threads; ++i) {
        mt_client_socket(sv_port_nr + i, &recv_ctx[i], &recv_sock[i]);
        sockets.push_back(recv_sock[i]);
      }

      std::thread sink_tid = std::thread(mt_sock_sink, &sockets);
      sink_tid.join();
    }
  }
}

void mt_client_socket(int32_t port_nr, zmq::context_t** c, zmq::socket_t** s) {
  *c = new zmq::context_t (1);
  *s = new zmq::socket_t (**c, ZMQ_PULL);
  zmq::socket_t & recv_sock = **s;

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << sv_dotted_quad << ":" << port_nr;

  int val = TCP_BUF_SIZE;
  recv_sock.setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

  recv_sock.connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  recv_sock.recv(&zmqmsg);
}

void mt_client(int32_t tid, int32_t port_nr,
               std::vector<boost::lockfree::spsc_queue<std::string*,
                           boost::lockfree::capacity<QUEUE_SIZE>>*>* queues) {
  int64_t ch_cli = openChannel("CPPDGClient          FPC_10MS", ASCII,
      NET_CONF);

  auto vec_q = (*queues);
  std::string* str;

  // Establish a receive socket
  zmq::context_t recv_ctx (1);
  zmq::socket_t recv_sock (recv_ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << sv_dotted_quad << ":" << port_nr;

  int val = TCP_BUF_SIZE;
  recv_sock.setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

  recv_sock.connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  recv_sock.recv(&zmqmsg);

  while (true) {
    zmq::message_t zmqmsg;
    recv_sock.recv(&zmqmsg);
    str = new std::string(static_cast<char*>(zmqmsg.data()));
    while ( !vec_q[tid]->push(str) ) ;
    logTS(ch_cli, 0);
  }
}

void mt_sock_sink(std::vector<zmq::socket_t*>* sockets) {
  int64_t ch_sink = openChannel("CPPDGSink            FPC_10MS", ASCII,
      NET_CONF);

  uint64_t i = 0;
  auto vec_s = (*sockets);
  uint64_t nr_q = vec_s.size();

  if (1 == nr_q) {
    do {
      zmq::message_t zmqmsg;
      if ( vec_s[0]->recv(&zmqmsg) ) logTS(ch_sink, 0);
    } while (true) ;
  }
  else if (2 == nr_q) {
    do {
      zmq::message_t zmqmsg;
      if ( vec_s[i]->recv(&zmqmsg) ) logTS(ch_sink, 0);
      i = !(i);
    } while (true) ;
  }
  else if (2 < nr_q) {
    do {
      zmq::message_t zmqmsg;
      if ( vec_s[i]->recv(&zmqmsg) ) logTS(ch_sink, 0);
      ++i;
      if (i == nr_q) i = 0;
    } while (true) ;
  }
}

void mt_spsc_sink(std::vector<boost::lockfree::spsc_queue<std::string *,
                         boost::lockfree::capacity<QUEUE_SIZE>>*>* queues) {
  int64_t ch_sink = openChannel("CPPDGSink            FPC_10MS", ASCII,
      NET_CONF);

  uint64_t i = 0;
  auto vec_q = (*queues);
  uint64_t nr_q = vec_q.size();
  std::string* str;

  if (1 == nr_q) {
    while (true) {
      if (vec_q[0]->pop(str)) {
        logTS(ch_sink, 0);
        delete str;
      }
    }
  }
  else if (2 == nr_q) {
    while (true) {
      if (vec_q[i]->pop(str)) {
        logTS(ch_sink, 0);
        delete str;
      }
      i = !(i);
    }
  }
  else if (2 < nr_q) {
    while (true) {
      if (vec_q[i]->pop(str)) {
        logTS(ch_sink, 0);
        delete str;
      }
      ++i;
      if (i == nr_q) i = 0;
    }
  }
}

void st_client() {
  int64_t ch_cli = openChannel("CPPDGClient          FPC_10MS", ASCII,
      NET_CONF);

  // Establish a receive socket
  zmq::context_t recv_ctx (1);
  zmq::socket_t recv_sock (recv_ctx, ZMQ_PULL);

  std::ostringstream ep_connect_url;
  ep_connect_url << "tcp://" << sv_dotted_quad << ":" << sv_port_nr;

  int val = TCP_BUF_SIZE;
  recv_sock.setsockopt(ZMQ_RCVBUF, &val, sizeof(val));

  recv_sock.connect (ep_connect_url.str());

  // Read initial message
  zmq::message_t zmqmsg;
  recv_sock.recv(&zmqmsg);

  while (true) {
    zmq::message_t zmqmsg;
    recv_sock.recv(&zmqmsg);
    logTS(ch_cli, 0);
  }
}

