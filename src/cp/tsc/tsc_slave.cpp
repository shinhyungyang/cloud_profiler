#include "time_tsc.h"

#include "tsc_messages.h"
#include "exports.h"
#include "serialization.h"
#include "globals.h"
#include "handlers.h"
#include "log.h"
#include "net_conf.h"

extern "C" {
#include "net.h"
#include "util.h"
}

#include "cxxopts.hpp"

#include <papi.h>
#include <zmq.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>

#include "query_tsc/query_tsc.h"

#define MEBIBYTE      (1024 * 1024)
#define TCP_BUF_SIZE  (MEBIBYTE * 12)

struct tsc_slave_info {
  char ipv4[MAX_IPV4_STR_SIZE + 1];
  int32_t port;
} info;

struct RTT_data {
  MSG_measure_result mm_res;
//struct timespec ts_stt;
//struct timespec ts_end;
  int64_t ts_stt;
  int64_t ns_stt;
  int64_t ts_end;
  int64_t ns_end;
  int64_t reply_cyc;
  int64_t reply_ns;
  uint32_t aux_stt;
  uint32_t aux_end;
  uint32_t reply_aux;
};

int64_t nr_iterpm;
//MSG_measure_result* res = nullptr;
RTT_data* RTTs = nullptr;
long long tsc_freq;

int32_t register_to_master(std::string sv_ip, int32_t sv_port_nr);
// Register to tsc_master

void get_commands(int32_t sv_port_nr);
// Get commands from tsc_master

int measure_tsc_rtt(bool isServer, MSG_measure_req* mm_req,
                    MSG_measure_result* mm_res, int64_t nr_iter);

int main (int argc, char ** argv) {

  // tsc master settings:
  std::string sv_dotted_quad;
  int32_t sv_port_nr;
  // logging to console setting:
  bool console_log = false;
  // parse command-line options:
  cxxopts::Options options("tsc_slave",
                           "cloud_profiler tsc slave");
  options.add_options()
  ("sv_ip",   "tsc master IP address", cxxopts::value<std::string>())
  ("sv_p",    "tsc master port number",
   cxxopts::value<int32_t>(sv_port_nr))
  ("c,console", "Enagle log-file output to console",
   cxxopts::value<bool>(console_log))
  ("h,help", "Usage information")
  ;

  auto option_result = options.parse(argc, argv);

  if (option_result.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  setLogFileName("/tmp/tsc_slave");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile();
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  // Set up data to request configuration:

  if (option_result.count("sv_ip")) {
    sv_dotted_quad = option_result["sv_ip"].as<std::string>();
  } else {
    sv_dotted_quad = tsc_master_default_ip;
  }
  if (!option_result.count("sv_p")) {
    sv_port_nr = tsc_master_default_port;
  }

  memset(info.ipv4, 0x0, sizeof(info.ipv4));
  getIPDottedQuad(info.ipv4, (size_t)MAX_IPV4_STR_SIZE);

  while (1) {
    int32_t newPort = register_to_master(sv_dotted_quad, sv_port_nr);
    get_commands(newPort);
  }

  return 0;
}

int32_t register_to_master(std::string sv_ip, int32_t sv_port_nr) {
  struct drand48_data dr48;
  int result;

  result = init_rand(&dr48);
  if (-1 == result) {
    exit(EXIT_FAILURE);
  }

  std::ostringstream ep;
  ep << "tcp://" << sv_ip << ":";
  ep << sv_port_nr;

  zmq::context_t context (1);
  zmq::socket_t sock (context, ZMQ_DEALER);

  char identity[10] = {};
  sprintf(identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));
  sock.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  sock.connect (ep.str());

  int rc;
  try {
    MSG_register_request mrr = {0,};
    strncpy(mrr.from_ip, info.ipv4, (size_t)MAX_IPV4_STR_SIZE);
    std::string file_path(__FILE__);
    std::string dir_path = file_path.substr(0, file_path.rfind("/"));
    double tsc_mhz = std::stod(exec((dir_path + "/query_tsc/query_tsc").c_str()));
    tsc_freq = (long long)(tsc_mhz * 1000000);
    mrr.tsc_freq = tsc_freq;
    zmq::message_t zmqmsg_req (&mrr, sizeof(MSG_register_request));

    rc = sock.send (zmqmsg_req);

    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
      exit(EXIT_FAILURE);
    }
  }
  catch (std::exception & e) {
    logFail(__FILE__, __LINE__, "Exception in DEALER/ROUTER connection");
    exit(EXIT_FAILURE);
  }

  zmq::message_t reply;
  zmq::pollitem_t items[] = {{static_cast<void*>(sock), 0, ZMQ_POLLIN, 0}};
  uint64_t msg_ctr = 0;
  try {
    do {
      zmq::poll(items, 1, 0);
      if (items[0].revents & ZMQ_POLLIN) {
        sock.recv (&reply);
        msg_ctr++;
      }
    } while (msg_ctr < 1); // try polling until a reply arrives
  }
  catch (std::exception & e) {
//  std::cout << "Exception in DEALER/ROUTER recv()" << std::endl;
//  std::cout << e.what() << std::endl;
  }

  if (reply.size() != sizeof(MSG_register_accept)) {
    logFail(__FILE__, __LINE__, "Registration failed");
    exit(EXIT_FAILURE);
  }
  MSG_register_accept * mra = static_cast<MSG_register_accept*>(reply.data());
  std::ostringstream oss;
  oss << "Registered to " << ep.str();
  logStatus(__FILE__, __LINE__, oss.str());
  oss.str("");
  oss.clear();
  oss << "Assigned port number: " << mra->port_number << std::endl;
  logStatus(__FILE__, __LINE__, oss.str());

  return mra->port_number;
}

void get_commands(int32_t sv_port_nr) {
  int rc;
  zmq::message_t zmqmsg;
  bool finished;

  std::ostringstream ep;
  ep << "tcp://*:" << sv_port_nr;

  zmq::context_t context (1);
  zmq::socket_t sock (context, ZMQ_REP);
  sock.bind (ep.str());

  while (1) {
    rc = sock.recv(&zmqmsg);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket receive", rc);
      exit(EXIT_FAILURE);
    }

    if (zmqmsg.size() != sizeof(MSG_measure_req)) {
      logFail(__FILE__, __LINE__, "Wrong message received");
      exit(EXIT_FAILURE);
    }

    MSG_measure_req * mm_req = static_cast<MSG_measure_req*>(zmqmsg.data());
    MSG_measure_result mm_res = {0,};

    if (mm_req->finished == 0) {
      finished = false;
    }
    else {
      finished = true;
    }

    if (false == finished) {
      bool isServer = false;
      if ((0 == strncmp(info.ipv4, mm_req->slave_from_ip, MAX_IPV4_STR_SIZE)) &&
          (mm_req->slave_from_port == sv_port_nr)) {
        // request
        isServer = true;
      }
      else if ((0 == strncmp(info.ipv4, mm_req->slave_to_ip, MAX_IPV4_STR_SIZE)) &&
          (mm_req->slave_to_port == sv_port_nr)) {
        // reply
        isServer = false;
      }

      nr_iterpm = mm_req->iter_per_measurement;
    //res = new MSG_measure_result[nr_iterpm];
      RTTs = new RTT_data[nr_iterpm];

      measure_tsc_rtt(isServer, mm_req, &mm_res, nr_iterpm);
      strncpy(mm_res.slave_from_ip, mm_req->slave_from_ip, MAX_IPV4_STR_SIZE);
      mm_res.slave_from_port = mm_req->slave_from_port;
      strncpy(mm_res.slave_to_ip, mm_req->slave_to_ip, MAX_IPV4_STR_SIZE);
      mm_res.slave_to_port = mm_req->slave_to_port;

      mm_res.finished = 0;
      zmqmsg = zmq::message_t (&mm_res, sizeof(MSG_measure_result));
    }
    else {
      mm_res.finished = 1;
      zmqmsg = zmq::message_t (&mm_res, sizeof(MSG_measure_result));
    }

    rc = sock.send(zmqmsg);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
      exit(EXIT_FAILURE);
    }

    if (false == finished) {
    //delete res;
      delete RTTs;
    }

    if (true == finished) {
      break;
    }
  }
}

int measure_tsc_rtt(bool isServer, MSG_measure_req* mm_req,
                    MSG_measure_result* mm_res, int64_t nr_iter) {

  int ret = -1;
  std::string dest_ip (mm_req->slave_to_ip, strlen(mm_req->slave_to_ip));
  int dest_port = tsc_rtt_meas_default_port;

  struct timespec ts_rtt_stt, ts_rtt_end, dur_ts;

  if (true == isServer) {
    struct drand48_data dr48;
    int result;
    std::ostringstream log;

    result = init_rand(&dr48);
    if (-1 == result) {
      log << "Error in initializing drand48: " << result;
      logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
      exit(EXIT_FAILURE);
    }
    char identity[10] = {};
    sprintf(identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));

    zmq::context_t context (1);
  //zmq::socket_t sock (context, ZMQ_REQ);
    zmq::socket_t sock (context, ZMQ_DEALER);
    int val = TCP_BUF_SIZE;
    sock.setsockopt(ZMQ_SNDBUF, &val, sizeof(val));
    sock.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
    std::ostringstream ep;
    ep << "tcp://" << dest_ip << ":" << dest_port;
    sock.connect (ep.str());

    int rc;
  //long long min_nsec  = LLONG_MAX, cur_nsec;  // used to find the minRTT
    long long min_ts = LLONG_MAX, cur_ts;       // used to find the minRTT

    long long from_cyc  = 0LL, end_cyc = 0LL;   // local cycles from the beginning
                                                // and the end of the round-trip

    long long from_ns = 0LL, end_ns = 0LL;      // local time from the beginning
                                                // and the end of the round-trip

    unsigned int from_aux = 0U, end_aux = 0U;   // TSC AUX for from_cyc and end_cyc

  //long long from_ns   = 0LL;                  // ns from local timespec at the
                                                // beginning of the round-trip

    long long to_cyc = 0LL, to_ns = 0LL;        // cycles and time from the
                                                // other server at the
                                                // (approximately) middle of
                                                // the round-trip.

    unsigned int to_aux = 0U;                   // TSC AUX for to_cyc

  //log << "Sending " << nr_iter << " synchronous REQ->REP messages";
    log << "Sending " << nr_iter << " synchronous DEALER->ROUTER messages";
    log << "...";
    logStatus(__FILE__, __LINE__, log.str()); log.str(""); log.clear();

    struct timespec now;
    char buf[1024];
    clock_gettime_tsc(CLOCK_REALTIME, &now);
    formatTS(&now, buf, 1024);
    std::ostringstream filename;
    filename << "/tmp/tsc_slave_rtt_log_" << buf << ".txt";
    std::ofstream logofs (filename.str(), std::ofstream::out);
    logofs << "# slave_1_address, slave_1_start_ns, slave_1_start_tsc, "
           << "slave_1_end_ns, slave_1_end_tsc, "
           << "slave_2_address, slave_2_ns, slave_2_tsc, min_rtt (ns), "
           << "slave_1_start_aux, slave_1_end_aux, slave_2_aux"
           << std::endl;

    TSC_req_cyc m;
  //struct timespec ts_stt;
  //struct timespec ts_end;
    struct timespec _ts_stt;
    struct timespec _ts_end;

    // Get time and tsc for calculating TSC frequency during the RTT measurement
    clock_gettime_tsc(CLOCK_REALTIME, &ts_rtt_stt);

    for (int64_t i = 0; i< nr_iter; i++) {

      zmq::message_t request((void *) &m, sizeof(TSC_req_cyc));
      zmq::message_t reply;

      // Start RTT measurement   ///////////////////////////////////////////////
    //clock_gettime_tsc(CLOCK_REALTIME, &ts_stt);
    //clock_gettime_tsc(CLOCK_REALTIME, &RTTs[i].ts_stt);
      clock_gettime_tsc(CLOCK_REALTIME, &_ts_stt);

      rc = sock.send (request);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
        exit(EXIT_FAILURE);
      }

      rc = sock.recv (&reply);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket receive", rc);
        exit(EXIT_FAILURE);
      }

    //clock_gettime_tsc(CLOCK_REALTIME, &ts_end);
    //clock_gettime_tsc(CLOCK_REALTIME, &RTTs[i].ts_end);
      clock_gettime_tsc(CLOCK_REALTIME, &_ts_end);

      RTTs[i].ns_stt = (int64_t) _ts_stt.tv_sec * (int64_t) NANO_S_PER_S;
      RTTs[i].ns_stt += (int64_t) _ts_stt.tv_nsec;
      RTTs[i].ts_stt = _ts_stt.tsc;
      RTTs[i].aux_stt = _ts_stt.aux;
      //RTTs[i].ts_stt = (int64_t)rdtscp(&aux);
      //RTTs[i].aux_stt = aux;

      RTTs[i].ns_end = (int64_t) _ts_end.tv_sec * (int64_t) NANO_S_PER_S;
      RTTs[i].ns_end += (int64_t) _ts_end.tv_nsec;
      RTTs[i].ts_end = _ts_end.tsc;
      RTTs[i].aux_end = _ts_end.aux;
      //RTTs[i].ts_end = (int64_t)rdtscp(&aux);
      //RTTs[i].aux_end = aux;
      // End of RTT measurement  ///////////////////////////////////////////////

      RTTs[i].reply_cyc = ((TSC_req_cyc *)reply.data())->reply_cyc;
      RTTs[i].reply_ns  = ((TSC_req_cyc *)reply.data())->reply_ns;
      RTTs[i].reply_aux = ((TSC_req_cyc *)reply.data())->reply_aux;
    }

    // Get time and tsc for calculating TSC frequency during the RTT measurement
    clock_gettime_tsc(CLOCK_REALTIME, &ts_rtt_end);

    for (int64_t i = 0; i< nr_iter; i++) {
      cur_ts = RTTs[i].ts_end - RTTs[i].ts_stt;
    //cur_nsec = ((RTTs[i].ts_end.tv_sec * NANO_S_PER_S) + RTTs[i].ts_end.tv_nsec)
    //         - ((RTTs[i].ts_stt.tv_sec * NANO_S_PER_S) + RTTs[i].ts_stt.tv_nsec);
      if (cur_ts < min_ts) {
        min_ts = cur_ts;
        from_cyc = RTTs[i].ts_stt;
        from_ns  = RTTs[i].ns_stt;
        end_cyc  = RTTs[i].ts_end;
        end_ns   = RTTs[i].ns_end;
        to_cyc   = RTTs[i].reply_cyc;
        to_ns    = RTTs[i].reply_ns;
        from_aux = RTTs[i].aux_stt;
        end_aux  = RTTs[i].aux_end;
        to_aux   = RTTs[i].reply_aux;
      }

      RTTs[i].mm_res.rtt_ns   = RTTs[i].ns_stt - RTTs[i].ns_end;
      RTTs[i].mm_res.from_cyc = RTTs[i].ts_stt; // RTTs[i].ts_stt.tsc;
      RTTs[i].mm_res.from_ns  = RTTs[i].ns_stt;
      RTTs[i].mm_res.end_cyc  = RTTs[i].ts_end; // RTTs[i].ts_end.tsc;
      RTTs[i].mm_res.end_ns   = RTTs[i].ns_end;
      RTTs[i].mm_res.to_cyc   = RTTs[i].reply_cyc;
      RTTs[i].mm_res.to_ns    = RTTs[i].reply_ns;

      RTTs[i].mm_res.from_aux = RTTs[i].aux_stt;
      RTTs[i].mm_res.end_aux  = RTTs[i].aux_end;
      RTTs[i].mm_res.to_aux   = RTTs[i].reply_aux;
    }

  //mm_res->rtt_ns    = min_nsec;
    mm_res->from_cyc  = from_cyc;
    mm_res->from_ns   = from_ns;
  //mm_res->from_ns   = from_ns;
    mm_res->to_cyc    = to_cyc;
    mm_res->to_ns     = to_ns;
    mm_res->end_cyc   = end_cyc;
    mm_res->end_ns    = end_ns;

    mm_res->from_aux  = from_aux;
    mm_res->to_aux    = to_aux;
    mm_res->end_aux   = end_aux;
    ret = 0;

    for (int64_t i = 0; i< nr_iter; i++) {
      std::ostringstream log;
      log << mm_req->slave_from_ip << ":" << mm_req->slave_from_port;
      log << ",";
      log << RTTs[i].mm_res.from_ns;
      log << ",";
      log << RTTs[i].mm_res.from_cyc;
      log << ",";
      log << RTTs[i].mm_res.end_ns;
      log << ",";
      log << RTTs[i].mm_res.end_cyc;
      log << ",";
      log << mm_req->slave_to_ip << ":" << mm_req->slave_to_port;
      log << ",";
      log << RTTs[i].mm_res.to_ns;
      log << ",";
      log << RTTs[i].mm_res.to_cyc;
      log << ",";
      log << RTTs[i].mm_res.end_ns - RTTs[i].mm_res.from_ns;
      log << ",";
      log << RTTs[i].mm_res.from_aux;
      log << ",";
      log << RTTs[i].mm_res.end_aux;
      log << ",";
      log << RTTs[i].mm_res.to_aux;
      log << std::endl;
      logofs << log.str();
    }

    logofs.close();
  }
  else {
    zmq::context_t context (1);
  //zmq::socket_t sock (context, ZMQ_REP);
    zmq::socket_t sock (context, ZMQ_ROUTER);
    int val = TCP_BUF_SIZE;
    sock.setsockopt(ZMQ_RCVBUF, &val, sizeof(val));
    std::ostringstream ep;
    ep << "tcp://*:" << dest_port;
    sock.bind (ep.str());

    std::ostringstream log;
    int rc;
  //uint64_t msg_ctr = 0;

    zmq::message_t identity;
    zmq::message_t msg;

    // Get time and tsc for calculating TSC frequency during the RTT measurement
    clock_gettime_tsc(CLOCK_REALTIME, &ts_rtt_stt);

    for (int64_t i = 0; i < nr_iter; i++) {
      struct timespec ts;
    //unsigned long long tsc;
    //unsigned int aux;

      rc = sock.recv (&identity);
      if (rc < 0) {
        log << "Error in ZMQ socket receive: " << rc;
        logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
        exit(EXIT_FAILURE);
      }

      rc = sock.recv (&msg);
      if (rc < 0) {
        log << "Error in ZMQ socket receive: " << rc;
        logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
        exit(EXIT_FAILURE);
      }

    //msg_ctr++;
    //if (msg.size() != sizeof(TSC_req_cyc)) {
    //  log << "Error: message of wrong size received";
    //  logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
    //  log << "Received: " << msg.size() << " expected: " << sizeof(TSC_req_cyc);
    //  logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
    //  log << "Message counter: " << msg_ctr;
    //  logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
    //  exit(EXIT_FAILURE);
    //}

      clock_gettime_tsc(CLOCK_REALTIME, &ts);
    //tsc = rdtscp(&aux);

      TSC_req_cyc * m = (TSC_req_cyc *)msg.data();
      m->reply_cyc = ts.tsc;
    //m->reply_cyc = tsc;
    //m->reply_aux = aux;
      m->reply_aux = ts.aux;
      m->reply_ns  = (int64_t) ts.tv_sec * (int64_t) NANO_S_PER_S;
      m->reply_ns += (int64_t) ts.tv_nsec;
      rc = sock.send(identity, ZMQ_SNDMORE);
      if (rc < 0) {
        log << "Error in ZMQ socket send: " << rc;
        logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
        exit(EXIT_FAILURE);
      }
      rc = sock.send(msg);
      if (rc < 0) {
        log << "Error in ZMQ socket send: " << rc;
        logFail(__FILE__, __LINE__, log.str()); log.str(""); log.clear();
        exit(EXIT_FAILURE);
      }
    }

    // Get time and tsc for calculating TSC frequency during the RTT measurement
    clock_gettime_tsc(CLOCK_REALTIME, &ts_rtt_end);

    mm_res->rtt_ns    = -1LL;
    mm_res->from_cyc  = -1LL;
    mm_res->to_cyc    = -1LL;
    ret = 0;
  }

  char tscfreq_buf[1024];
  formatTS(&ts_rtt_stt, tscfreq_buf, sizeof(tscfreq_buf));
  std::ostringstream tscfreq_filename;
  tscfreq_filename << "/tmp/tsc_slave_tscfreq_" << tscfreq_buf << ".txt";
  subtractTS(&dur_ts, &ts_rtt_stt, &ts_rtt_end);

  int64_t stt_ns = (ts_rtt_stt.tv_sec * NANO_S_PER_S) + ts_rtt_stt.tv_nsec;
  int64_t end_ns = (ts_rtt_end.tv_sec * NANO_S_PER_S) + ts_rtt_end.tv_nsec;

  std::ostringstream log;
  log << "# Core No, system clock (ns), TSC (cyc)" << std::endl;

  log << ts_rtt_stt.aux;
  log << ",";
  log << stt_ns;
  log << ",";
  log << ts_rtt_stt.tsc;
  log << std::endl;

  log << ts_rtt_stt.aux;
  log << ",";
  log << end_ns;
  log << ",";
  log << ts_rtt_end.tsc;
  log << std::endl;

  std::ofstream tscfreq_logofs(tscfreq_filename.str(), std::ofstream::out);
  tscfreq_logofs << log.str();
  tscfreq_logofs.close();

  return ret;
}

