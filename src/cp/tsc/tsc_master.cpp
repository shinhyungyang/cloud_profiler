#include "time_tsc.h"

#include "globals.h"
#include "tsc_messages.h"
#include "exports.h"
#include "serialization.h"
#include "handlers.h"
#include "log.h"

#include "cxxopts.hpp"
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <zmq.hpp>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <fstream>
#include <sys/select.h>

class MeasureResult {
public:
  std::string from_ip;
  int from_port;
  std::string to_ip;
  int to_port;
  int64_t rtt_ns;
  int64_t from_cyc;
  int64_t from_ns;
  int64_t to_cyc;
  int64_t to_ns;
  int64_t end_cyc;
  int64_t end_ns;
  uint32_t from_aux;
  uint32_t to_aux;
  uint32_t end_aux;

  bool operator<(const MeasureResult &rhs) const { return rtt_ns < rhs.rtt_ns; }
};

std::vector<MeasureResult> measResultList;

void slave_registration(std::unordered_map<std::string, std::tuple<std::string,
    int32_t, zmq::socket_t*, int64_t>> & slaves, int32_t port_number,
    int32_t nr_slaves, std::ofstream & ofs_reg);
// wait for nr_slaves to subscribe to this tsc_master

void request_measurement(std::unordered_map<std::string,
    std::tuple<std::string, int32_t, zmq::socket_t*, int64_t>> & slaves,
    std::ofstream & ofs);
// TODO: Request an RTT measurement to a pair of slaves

static bool checkTerminate();

// Arguments to read from users
int32_t port_number = -1;
int32_t nr_slaves = 0;
bool console_log = false;
uint32_t interval = 0;
uint32_t iteration = 0;
uint32_t nr_iterpm = 100;
bool periodic = false;
bool nonstop = false;

int main(int argc, char ** argv) {

  cxxopts::Options options("tsc_master",
                           "cloud_profiler tsc master");
  options.add_options()
  ("c,console",     "Write log-file output to console",
   cxxopts::value<bool>(console_log))
  ("p,port",        "Port number",
   cxxopts::value<int32_t>(port_number))
  ("n,nr_slaves",   "The number of slaves to subscribe to this tsc_master",
   cxxopts::value<int32_t>(nr_slaves))
  ("e,periodic",    "Periodically do measurements",
   cxxopts::value<bool>(periodic))
  ("t,interval",    "Time duration of each period in seconds",
   cxxopts::value<uint32_t>(interval))
  ("i,iteration",   "The number of measurements to do",
   cxxopts::value<uint32_t>(iteration))
  ("m,iter_per_measurement",   "The number of iterations per measurement",
   cxxopts::value<uint32_t>(nr_iterpm))
  ("h,help",        "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up logging:
  setLogFileName("/tmp/tsc_master");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile();
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  if (periodic == false) {
    if (iteration == 0) {
      iteration = 1;
    }
    nonstop   = false;
  }

  if (periodic == true && iteration == 0) {
    // when periodic argument is given but no iteration, then run infinitely
    nonstop = true;
  }

  if (port_number == -1) {
    port_number = tsc_master_default_port;
  }

  const struct timespec TS_SLEEP  = { .tv_sec = 1, .tv_nsec = 0 };
  const struct timespec TS_BUDGET = { .tv_sec = interval, .tv_nsec = 0 };
  // TODO: condition for the loop
  while (false == checkTerminate()) {
    struct timespec now, budgetEnd;
    char buf[1024];

    clock_gettime_tsc(CLOCK_REALTIME, &now);
    addTS(&budgetEnd, &now, &TS_BUDGET);

    // do the work
    formatTS(&now, buf, 1024);
    std::ostringstream filename1;
    std::ostringstream filename2;
    filename1 << "/tmp/tsc_master_register_log_" << buf << ".txt";
    filename2 << "/tmp/tsc_master_minrtt_log_" << buf << ".txt";
    std::ofstream logofs_reg (filename1.str(), std::ofstream::out);
    std::ofstream logofs (filename2.str(), std::ofstream::out);
    logofs_reg << "# slave_ip, slave_port, tsc_freq" << std::endl;
    logofs << "# slave_1_address, slave_1_start_ns, slave_1_start_tsc, "
           << "slave_1_end_ns, slave_1_end_tsc, "
           << "slave_2_address, slave_2_ns, slave_2_tsc, min_rtt (ns), "
           << "slave_1_start_aux, slave_1_end_aux, slave_2_aux"
           << std::endl;
    std::unordered_map<std::string, std::tuple<std::string, int32_t,
      zmq::socket_t*, int64_t>> slaves;
    slave_registration(slaves, port_number, nr_slaves, logofs_reg);
    request_measurement(slaves, logofs);
    measResultList.clear();
    logofs_reg.close();
    logofs.close();

    iteration--;
  //if (iteration <= 1) {
    if (iteration <= 6) {
    //nr_iterpm = 1000 * 1000;
    //nr_iterpm = 100 * 100 * 4 * 6;
    //nr_iterpm = 100 * 100 * 4;
    //nr_iterpm = 100 * 100;
      nr_iterpm = 100;
    }

    do {
      pselect(0, NULL, NULL, NULL, &TS_SLEEP, NULL);
      clock_gettime_tsc(CLOCK_REALTIME, &now);
    } while (compareTS(&now, &budgetEnd) < 0) ;
    addTS(&budgetEnd, &budgetEnd, &TS_BUDGET);

  }
  closeStatusLogFile();
  return 0;
}

void slave_registration(std::unordered_map<std::string, std::tuple<std::string,
    int32_t, zmq::socket_t*, int64_t>> & slaves, int32_t port_number,
    int32_t nr_slaves, std::ofstream & ofs_reg) {
  int rc;
  int32_t msg_ctr = 0;
  std::string recvStr;
  std::string sendStr;

  // Construct zmq endpoint:
  std::ostringstream ep;
  ep << "tcp://*:" << port_number;

  // Setup a zmq context:
  // (One instance is sufficient for multiple connections)
  zmq::context_t context (1);
  zmq::socket_t sock (context, ZMQ_ROUTER);

  sock.bind(ep.str());

  zmq::message_t identity;
  zmq::message_t zmqmsg;

  try {
    while (msg_ctr < nr_slaves) {

      rc = sock.recv (&identity);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket receive");
        exit(EXIT_FAILURE);
      }
      rc = sock.recv (&zmqmsg);
      msg_ctr++;
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket receive");
        exit(EXIT_FAILURE);
      }

      MSG_register_request * mrr = static_cast<MSG_register_request*>(zmqmsg.data());
      std::string cur_ip(mrr->from_ip, strlen(mrr->from_ip));

      /**
       * Add the slave to a hashmap
       */
      std::unordered_map<std::string, std::tuple<std::string, int32_t,
        zmq::socket_t*, int64_t>>::const_iterator got;
      std::ostringstream slave_id;
      int availablePortFrom = tsc_command_default_port;
      int32_t cur_port;
      do {
        cur_port = availablePortFrom++;
        slave_id.str(""); slave_id.clear();
        slave_id << cur_ip << ":" << cur_port;
        got = slaves.find(slave_id.str());
      }
      while ( got != slaves.end() );

      /**
       * Notify of port number to the slave
       */
      MSG_register_accept mra;
      mra.port_number = cur_port;
      zmq::message_t zmqmsg_reply (&mra, sizeof(MSG_register_accept));

      rc = sock.send(identity, ZMQ_SNDMORE);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
        exit(EXIT_FAILURE);
      }
      rc = sock.send(zmqmsg_reply);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
        exit(EXIT_FAILURE);
      }

      std::tuple<std::string, int32_t, zmq::socket_t*, int64_t> newPair (
          cur_ip, cur_port, NULL, mrr->tsc_freq);
      slaves.insert ( std::make_pair(slave_id.str(), newPair) );

      std::ostringstream log;
      log << slave_id.str() << "," << mrr->tsc_freq << std::endl;
      logStatus(__FILE__, __LINE__, log.str());
      ofs_reg << log.str();
    }
  }
  catch (std::exception & e) {
    logFail(__FILE__, __LINE__, "Exception in DEALER/ROUTER connection");
    exit(EXIT_FAILURE);
  }
//std::cout << "aaa registration finished" << std::endl;
}

// TODO the scope of its functionality is bigger than what is intended.
void request_measurement(std::unordered_map<std::string,
    std::tuple<std::string, int32_t, zmq::socket_t*, int64_t>> & slaves,
    std::ofstream & ofs) {
  int rc;
  std::string recvStr;
  std::string sendStr;
  zmq::message_t zmqmsg;

  // Establish connections to all tsc_slaves at the beginning of the function
  zmq::context_t context (1);
  for ( auto it = slaves.begin(); it != slaves.end(); ++it ) {
    std::get<2>(std::get<1>(*it)) = new zmq::socket_t(context, ZMQ_REQ);
    std::ostringstream ep;
    ep << "tcp://" << std::get<0>(std::get<1>(*it)) << ":";
    ep << std::get<1>(std::get<1>(*it));
    std::get<2>(std::get<1>(*it))->connect (ep.str());
  }


  /**
   * Add all possible unique pairs considering the order
   */
  std::vector<std::tuple<std::tuple<std::string, int32_t, zmq::socket_t*, int64_t>, std::tuple<std::string, int32_t, zmq::socket_t*, int64_t>>> pairs;
  for ( auto it_1 = slaves.begin(); it_1 != slaves.end(); ++it_1 ) {
    for ( auto it_2 = slaves.begin(); it_2 != slaves.end(); ++it_2 ) {
      if (0 != std::get<0>(*it_1).compare(std::get<0>(*it_2)))
      pairs.push_back(std::make_tuple(std::get<1>(*it_1), std::get<1>(*it_2)));
    }
  }

  try {
    for (auto it = pairs.begin(); it != pairs.end(); ++it) {
      auto sender = std::get<0>(*it);
      auto receiver = std::get<1>(*it);

      std::vector<std::tuple<std::string, int32_t, zmq::socket_t*, int64_t>> targets;
      targets.push_back(std::get<0>(*it));
      targets.push_back(std::get<1>(*it));

      zmq::context_t context (1);
      std::vector<zmq::socket_t*> socks;

      /**
       * Send a request to each of a pair of all slaves registered
       */
      for (auto it = targets.begin(); it != targets.end(); ++it) {

        std::ostringstream slave_id;
        slave_id << std::get<0>(*it) << ":" << std::get<1>(*it);
        auto got = slaves.find(slave_id.str());
        if (got == slaves.end()) {
          std::ostringstream log;
          log << "Slave \"" << slave_id.str() << "\" is not found.";
          logFail(__FILE__, __LINE__, log.str());
          exit(EXIT_FAILURE);
        }

        auto & tup = std::get<1>(*got); // `&' lets the reference modifiable
        if (NULL == std::get<2>(tup)) {
          logFail(__FILE__, __LINE__, "Socket should have been prepared.");
          exit(EXIT_FAILURE);
        }

        socks.push_back(std::get<2>(tup));
        zmq::socket_t * sock = socks[socks.size()-1];
        {
          MSG_measure_req mmr = {0,};
          strncpy(mmr.slave_from_ip, std::get<0>(sender).c_str(), MAX_IPV4_STR_SIZE);
          mmr.slave_from_port = std::get<1>(sender);
          strncpy(mmr.slave_to_ip, std::get<0>(receiver).c_str(), MAX_IPV4_STR_SIZE);
          mmr.slave_to_port = std::get<1>(receiver);
          mmr.finished = 0;
          mmr.iter_per_measurement = nr_iterpm;

          zmqmsg = zmq::message_t (&mmr, sizeof(MSG_measure_req));
          rc = sock->send(zmqmsg);
          if (rc < 0) {
            logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
            exit(EXIT_FAILURE);
          }
        }
      }

      zmq::pollitem_t items[socks.size()];
      for (size_t i=0; i<socks.size(); ++i) {
        items[i].socket = static_cast<void*>(*socks[i]);
        items[i].fd = 0;
        items[i].events = ZMQ_POLLIN;
      }

      int numSocketsToPoll =  socks.size();
      while (numSocketsToPoll > 0) {
        int numPolled = zmq::poll(items, socks.size(), -1);
        for (size_t sock_idx=0; sock_idx<socks.size(); ++sock_idx) {
          if (items[sock_idx].revents & ZMQ_POLLIN) {
            socks[sock_idx]->recv (&zmqmsg);

            MSG_measure_result * mma = static_cast<MSG_measure_result*>(zmqmsg.data());
            if (sizeof(MSG_measure_result) != zmqmsg.size()) {
              logFail(__FILE__, __LINE__, "Wrong message received");
              exit(EXIT_FAILURE);
            }

            std::ostringstream slave_id;
            slave_id << mma->slave_from_ip << ":" << mma->slave_from_port;
            auto got = slaves.find(slave_id.str());
            auto & tup = std::get<1>(*got); // `&' lets the reference modifiable
            //int64_t tsc_freq = std::get<3>(tup);
            //int64_t dur_ts  = mma->end_cyc - mma->from_cyc;

            std::ostringstream log;
            if (mma->from_cyc != -1)
          //if (mma->rtt_ns != -1)
            {
              MeasureResult mr;
              mr.from_ip = mma->slave_from_ip;
              mr.from_port = mma->slave_from_port;
              mr.to_ip = mma->slave_to_ip;
              mr.to_port = mma->slave_to_port;
              mr.rtt_ns = mma->end_ns - mma->from_ns;
              mr.from_cyc = mma->from_cyc;
              mr.from_ns = mma->from_ns;
              mr.to_cyc = mma->to_cyc;
              mr.to_ns  = mma->to_ns;
              mr.end_cyc = mma->end_cyc;
              mr.end_ns  = mma->end_ns;

              mr.from_aux = mma->from_aux;
              mr.to_aux = mma->to_aux;
              mr.end_aux = mma->end_aux;

              measResultList.push_back(mr);
            }
          }
        }
        numSocketsToPoll -= numPolled;
      }
    }

    // send ``finish'' to all slaves
    for ( auto it = slaves.begin(); it != slaves.end(); ++it ) {
      zmq::socket_t * sock = std::get<2>(std::get<1>(*it));
      MSG_measure_req mmr = {0,};
      mmr.finished = 1;
      zmq::message_t zmqmsg = zmq::message_t (&mmr, sizeof(MSG_measure_req));
      int rc = sock->send(zmqmsg);
      if (rc < 0) {
        logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
        exit(EXIT_FAILURE);
      }
      zmq::message_t recv;
      sock->recv(&recv);
      MSG_measure_result * mma = static_cast<MSG_measure_result*>(recv.data());
      if (mma->finished == 0) {
        std::ostringstream log;
        log << "Wrong message: ";
        log << "The current RTT session should have been finished here.";
        logFail(__FILE__, __LINE__, log.str());
      }
    }
  }
  catch (std::exception & e) {
    std::ostringstream log;
    log << "Exception: " << e.what();
    logFail(__FILE__, __LINE__, log.str());
  }
  std::sort(measResultList.begin(), measResultList.end());
  for ( auto it = measResultList.begin(); it != measResultList.end(); ++it ) {
    std::ostringstream log;
    log << it->from_ip << ":" << it->from_port;
    log << ",";
    log << it->from_ns;
    log << ",";
    log << it->from_cyc;
    log << ",";
    log << it->end_ns;
    log << ",";
    log << it->end_cyc;
    log << ",";
    log << it->to_ip << ":" << it->to_port;
    log << ",";
    log << it->to_ns;
    log << ",";
    log << it->to_cyc;
    log << ",";
    log << it->rtt_ns;

    log << ",";
    log << it->from_aux;
    log << ",";
    log << it->end_aux;
    log << ",";
    log << it->to_aux;

    ofs << log.str() << std::endl;
  }
  for ( auto it = slaves.begin(); it != slaves.end(); ++it ) {
    delete std::get<2>(std::get<1>(*it));
  }
}

static bool checkTerminate() {
  if (nonstop || (!nonstop && iteration > 0)) {
    // do not evaluate when nonstop
    return false;
  }
  return true;
}
