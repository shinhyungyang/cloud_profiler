#include "tsc_messages.h"
#include "tsc.h"
#include "globals.h"
#include "handlers.h"
#include "log.h"

extern "C" {
#include "util.h"
#include "net.h"
}

#include "cxxopts.hpp"

#include <papi.h>
#include <zmq.hpp>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>

void do_bulk_sync_send_and_recv(zmq::socket_t & sock, int64_t nr_iter);
// Send 'nr_iter' synchronous REQ->REPL messages and observe the received RTTs. 

void do_bulk_async_send_and_recv(zmq::socket_t & sock, int64_t nr_iter, int64_t bulk_sz);
// Send 'nr_iter' asynchronous messages and observe the received RTTs. 

void do_bulk_async_send(zmq::socket_t & sock, int64_t nr_iter, int64_t bulk_sz);
// Send 'nr_iter' asynchronous messages

void do_bulk_async_recv(zmq::socket_t & sock, int64_t nr_iter);
// Receive 'nr_iter" asynchronous messages and observe the RTTs.


void async_send_and_recv(zmq::context_t & ctx, std::ostringstream & ep, int nr_iterations, int64_t bulk_sz);
void async_send(zmq::context_t & ctx, std::ostringstream & ep, int nr_iterations, int64_t bulk_sz);
void async_recv(zmq::context_t & ctx, uint32_t sv_port_nr, int nr_iterations);

// tsc master settings:
std::string sv_dotted_quad;

int main (int argc, char ** argv) {
  
  uint32_t sv_port_nr;
  // logging to console setting:
  bool console_log = false;
  // asynchronous messaging:
  bool async = false;
  // user-defined bulk size:
  int64_t bulk_sz = -1;
  // use multiple sockets:
  bool multisocket = false;
  // parse command-line options:
  cxxopts::Options options("tsc_slave_1",
                           "cloud_profiler tsc slave 1");
  options.add_options()
  ("sv_ip",           "tsc slave 2 IP address", cxxopts::value<std::string>())
  ("sv_p",            "tsc slave 2 port number",
   cxxopts::value<uint32_t>(sv_port_nr))
  ("b_size",          "Size of a bulk to send and receive at a time",
   cxxopts::value<int64_t>(bulk_sz))
  ("a,async",         "Use asynchronous tsc messages (DEALER-ROUTER)",
   cxxopts::value<bool>(async))
  ("m,multi_socket",  "Use multiple sockets",
   cxxopts::value<bool>(multisocket))
  ("c,console",       "Enable log-file output to console",
   cxxopts::value<bool>(console_log))
  ("i,iterations",    "Number of iterations (sent messages)",
   cxxopts::value<std::string>()->default_value("1KB"))
  ("h,help",          "Usage information")
  ;

  auto option_result = options.parse(argc, argv);

  if (option_result.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    std::cout << "number_of_iterations accepts postfix modifiers: "
              << std::endl;
    std::cout << "KB, KiB, MB, MiB, GB, GiB" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string iterstring = option_result["iterations"].as<std::string>();
  int nr_iterations = convertA2I(iterstring.c_str());

  setLogFileName("/tmp/tsc_slave_1");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile(); 

  // Set up PAPI:
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT)
    logFail( __FILE__, __LINE__, "PAPI_library_init", retval );

  if (!check_rdtsc()) {
    std::cout << "ERROR: RDTSC instruction not supported, exiting...";
    std::cout << std::endl;
    exit(EXIT_FAILURE);
  }


  // Set up master's IP address and port:
  if (option_result.count("sv_ip")) {
    sv_dotted_quad = option_result["sv_ip"].as<std::string>();
  } else {
    sv_dotted_quad = tsc_master_default_ip;
  }
  if (!option_result.count("sv_p")) {
    sv_port_nr = tsc_master_default_port;
  }

  // zmq:
  // A 0MQ context is thread safe and may be shared among as many application
  // threads as necessary, without any additional locking required on the part
  // of the caller.
  zmq::context_t context (1);

  // talk to master first
  // Set address for socket "IP:port"
  std::ostringstream ep;
  ep << "tcp://" << sv_dotted_quad << ":";
  ep << sv_port_nr;

  if (async && multisocket) {
    // Separated sockets in a dedicated threads for asynchronous communication
    std::thread thr_async_send;
    std::thread thr_async_recv;

    // two-sockets, multi-threaded
    thr_async_send = std::thread(async_send, std::ref(context),
        std::ref(ep), nr_iterations, bulk_sz);
    thr_async_recv = std::thread(async_recv, std::ref(context),
        sv_port_nr, nr_iterations);

    thr_async_send.join();
    thr_async_recv.join();
  }
  else if (async && !multisocket) {
    // single-socket, but using a separate thread
    std::thread thr_send_and_recv;
    thr_send_and_recv = std::thread(async_send_and_recv, std::ref(context),
        std::ref(ep), nr_iterations, bulk_sz);

    thr_send_and_recv.join();
  }
  else if (!async && !multisocket) {
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect (ep.str());

    do_bulk_sync_send_and_recv(socket, nr_iterations);
  }

  closeStatusLogFile();

  return 0;
}

std::string makeLogFileName(const std::string& name) {
  char tmp_buf[255];
  if (0 != getIPDottedQuad(tmp_buf, 255)) {
    //FIXME: need a strategy what to do if we fail here.
  }
  pid_t pid = getpid();
  std::ostringstream name_buf;  
  name_buf << name;
  name_buf << ":" << tmp_buf; // IP address
  name_buf << ":" << pid << ".txt";
  return name_buf.str();
}

void do_bulk_sync_send_and_recv(zmq::socket_t & sock, int64_t nr_iter) {
  int rc;
  int64_t min_cyc = LLONG_MAX, cur_cyc;
  int64_t min_nsec = LLONG_MAX, cur_nsec;
  int64_t overall_start_nsec, overall_end_nsec;
  int64_t accum_nsec = 0LL, reply_cyc = 0LL;
  std::cout << "Sending " << nr_iter << " synchronous REQ->REP messages";
  std::cout << "..." << std::endl;
  overall_start_nsec = PAPI_get_real_nsec();
  std::ofstream logFile(makeLogFileName("/tmp/tsc_slave_1_data_to_"+sv_dotted_quad));
  logFile << "req_ns_send, reply_ns_receive, reply_ns_send, req_ns_receive\n";
  TSC_msg m;
  for (int64_t i = 0; i< nr_iter; i++) {
    m.req_cyc_start = PAPI_get_real_cyc();
    m.req_nsec_start = PAPI_get_real_nsec();
    zmq::message_t request((void *) &m, sizeof(TSC_msg));
    rc = sock.send (request);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket send" << std::endl;
      exit(EXIT_FAILURE);
    }
    zmq::message_t reply;
    sock.recv (&reply);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    TSC_msg * msg = (TSC_msg *) reply.data();
    int64_t req_nsec_end = PAPI_get_real_nsec();
    cur_cyc = PAPI_get_real_cyc() - msg->req_cyc_start;
    cur_nsec = req_nsec_end - msg->req_nsec_start; 
    min_cyc = std::min(min_cyc, cur_cyc);
    if (cur_nsec < min_nsec) {
      min_nsec = cur_nsec;
      reply_cyc = msg->reply_cyc;
    }
    logFile << msg->req_nsec_start << "," 
      << msg->reply_nsec_start  << "," 
      << msg->reply_nsec_end  << "," 
      << req_nsec_end  << "\n"; 
    accum_nsec += cur_nsec;
  }
  logFile.close();
  // log summary information(minrtt, mincyc ...)
  logFile.open(makeLogFileName("/tmp/tsc_slave_1_summary_to_"+sv_dotted_quad));
  // signaling slave 2 to be shutdown
  m.req_nsec_start = 0;
  zmq::message_t request((void *) &m, sizeof(TSC_msg));
  rc = sock.send (request);
  // calculate summary informations
  overall_end_nsec = PAPI_get_real_nsec();
  int64_t overall_nsec = overall_end_nsec - overall_start_nsec;
  int64_t avg_nsec = accum_nsec / nr_iter;
  char buf[1024];
  formatDuration((uint64_t) avg_nsec, buf, 1024, true);
  std::cout << "Avg RTT/request: " <<  buf << std::endl;
  logFile << "Avg RTT/request: " <<  buf << std::endl;
  formatDuration((uint64_t) min_nsec, buf, 1024, true);
  std::cout << "Min RTT/request: " << buf << std::endl;
  std::cout << "Min cyc/request: " << l2aCommas(min_cyc, buf) << std::endl;
  std::cout << "Min reply cycle: " << l2aCommas(reply_cyc, buf) << std::endl;
  logFile << "Min RTT/request: " << buf << std::endl;
  logFile << "Min cyc/request: " << l2aCommas(min_cyc, buf) << std::endl;
  logFile << "Min reply cycle: " << l2aCommas(reply_cyc, buf) << std::endl;
  formatDuration((uint64_t) overall_nsec, buf, 1024, true);
  std::cout << "Elapsed Time: " << buf << std::endl;
  logFile << "Elapsed Time: " << buf << std::endl;
  logFile.close();
}

long long g_overall_start_nsec;

void do_bulk_async_send_and_recv(zmq::socket_t & sock, int64_t nr_iter, int64_t bulk_sz) {
  int rc;
  long long min_cyc = LLONG_MAX, cur_cyc;
  long long min_nsec = LLONG_MAX, cur_nsec;
  long long overall_start_nsec, overall_end_nsec;
  long long accum_nsec = 0LL, reply_cyc = 0LL;
  zmq::pollitem_t items[] = {{sock, 0, ZMQ_POLLIN, 0}};
  zmq::message_t reply;
  int64_t recv_ctr = 0, sent_ctr = 0;
  int64_t to_send = 0, to_recv = 0, received = 0;
  bool fail_fast = false;

  if (-1 == bulk_sz) {
    fail_fast = true;
  }

  std::cout << "Sending " << nr_iter << " asynchronous DEALER->ROUTER messages";
  std::cout << "..." << std::endl;
  std::cout << "(user-defined bulk size: " << bulk_sz << ")" << std::endl;
  overall_start_nsec = PAPI_get_real_nsec();
  TSC_msg m;
  try {
    while (true) {
      /////////////////////////////////////////////////////////////////////////
      if (recv_ctr >= nr_iter) {
        break;
      }
      /////////////////////////////////////////////////////////////////////////
      if (sent_ctr < nr_iter) {
        if (fail_fast) {
          to_send = 1;
        } else if (nr_iter - sent_ctr >= bulk_sz) {
          to_send = bulk_sz;
        } else {
          to_send = nr_iter - sent_ctr;
        }
        for (int64_t i = 0; i < to_send; i++) {
          m.req_cyc_start = PAPI_get_real_cyc();
          m.req_nsec_start = PAPI_get_real_nsec();
          zmq::message_t request((void *) &m, sizeof(TSC_msg));
          rc = sock.send (request);
          ++sent_ctr;
          if (rc < 0) {
            std::cout << "Error in ZMQ socket send" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
      }
      /////////////////////////////////////////////////////////////////////////
      to_recv = 0;
      received = 0;
      if (fail_fast) {
        to_recv = 1;
      } else if (nr_iter - recv_ctr >= bulk_sz) {
        to_recv = bulk_sz;
      } else {
        to_recv = nr_iter - recv_ctr;
      }

      do {
        zmq::poll(items, 1, 0);
        if (items[0].revents & ZMQ_POLLIN) {
          sock.recv (&reply);
          ++recv_ctr;
          ++received;
          TSC_msg * msg = (TSC_msg *)reply.data();
          cur_cyc = PAPI_get_real_cyc() - msg->req_cyc_start;
          cur_nsec = PAPI_get_real_nsec() - msg->req_nsec_start; 
          min_cyc = std::min(min_cyc, cur_cyc);
          if (cur_nsec < min_nsec) {
            min_nsec = cur_nsec;
            reply_cyc = msg->reply_cyc;
          }
          accum_nsec += cur_nsec;
      } else {
      }
      } while (!fail_fast && received < to_recv);
      /////////////////////////////////////////////////////////////////////////
    }
  }
  catch (std::exception & e) {
    std::cout << "Error in do_bulk_async_send_and_recv()" << std::endl;
    std::cout << e.what() << std::endl;
  }
  overall_end_nsec = PAPI_get_real_nsec();


  long long overall_nsec = overall_end_nsec - overall_start_nsec;
  //long long avg_nsec = (overall_end_nsec - overall_start_nsec) / recv_ctr;
  long long avg_nsec = accum_nsec / recv_ctr;
  char buf[1024];
  formatDuration((uint64_t) avg_nsec, buf, 1024, true);
  std::cout << "Avg RTT/request: " <<  buf << std::endl; 
  formatDuration((uint64_t) min_nsec, buf, 1024, true);
  std::cout << "Min RTT/request: " << buf << std::endl;
  std::cout << "Min cyc/request: " << l2aCommas(min_cyc, buf) << std::endl;
  std::cout << "Min reply cycle: " << l2aCommas(reply_cyc, buf) << std::endl;
  formatDuration((uint64_t) overall_nsec, buf, 1024, true);
  std::cout << "Elapsed Time: " << buf << std::endl;
}

void do_bulk_async_send(zmq::socket_t & sock, int64_t nr_iter, int64_t bulk_sz) {
  int rc;
  int64_t sent_ctr = 0;
  int64_t to_send = 0;
  bool fail_fast = false;

  if (-1 == bulk_sz) {
    fail_fast = true;
  }

  std::cout << "Sending " << nr_iter << " asynchronous DEALER->ROUTER messages";
  std::cout << "..." << std::endl;
  std::cout << "(user-defined bulk size: " << bulk_sz << ")" << std::endl;
  g_overall_start_nsec = PAPI_get_real_nsec();
  TSC_msg m;
  try {
    while (true) {
      if (sent_ctr >= nr_iter) {
        break;
      }
      if (sent_ctr < nr_iter) {
        if (fail_fast) {
          to_send = 1;
        } else if (nr_iter - sent_ctr >= bulk_sz) {
          to_send = bulk_sz;
        } else {
          to_send = nr_iter - sent_ctr;
        }
        for (int64_t i = 0; i < to_send; i++) {
          m.req_cyc_start = PAPI_get_real_cyc();
          m.req_nsec_start = PAPI_get_real_nsec();
          zmq::message_t request((void *) &m, sizeof(TSC_msg));
          rc = sock.send (request);
          ++sent_ctr;
          if (rc < 0) {
            std::cout << "Error in ZMQ socket send" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
      }
    }
  }
  catch (std::exception & e) {
    std::cout << "Error in do_bulk_async_send()" << std::endl;
    std::cout << e.what() << std::endl;
  }
}

void do_bulk_async_recv(zmq::socket_t & sock, int64_t nr_iter) {
  int rc;

  long long min_cyc = LLONG_MAX, cur_cyc;
  long long min_nsec = LLONG_MAX, cur_nsec;
  long long overall_end_nsec;
  long long accum_nsec = 0LL, reply_cyc = 0LL;
  zmq::message_t identity;
  zmq::message_t reply;
  int64_t recv_ctr = 0;

  try {
    while (true) {

      if (recv_ctr >= nr_iter) {
        break;
      }

      rc = sock.recv (&identity);
      if (rc < 0) {
        std::cout << "Error in ZMQ socket receive" << std::endl;
        exit(EXIT_FAILURE);
      }

      rc = sock.recv (&reply);
      recv_ctr++;
      if (rc < 0) {
        std::cout << "Error in ZMQ socket receive" << std::endl;
        exit(EXIT_FAILURE);
      }

      if (reply.size() != sizeof(TSC_msg)) {
        std::cout << "Error: message of wrong size received" << std::endl;
        std::cout << "Received: " << reply.size() << " expected: " << sizeof(TSC_msg) << std::endl;
        std::cout << "Message counter: " << recv_ctr << std::endl;
        exit(EXIT_FAILURE);
      } 

      TSC_msg * msg = (TSC_msg *)reply.data();

      cur_cyc = PAPI_get_real_cyc() - msg->req_cyc_start;
      cur_nsec = PAPI_get_real_nsec() - msg->req_nsec_start; 
      min_cyc = std::min(min_cyc, cur_cyc);
      if (cur_nsec < min_nsec) {
        min_nsec = cur_nsec;
        reply_cyc = msg->reply_cyc;
      }
      accum_nsec += cur_nsec;
    }
  }
  catch (std::exception & e) {
    std::cout << "Error in do_bulk_async_recv()" << std::endl;
    std::cout << e.what() << std::endl;
  }
  overall_end_nsec = PAPI_get_real_nsec();

  long long overall_nsec = overall_end_nsec - g_overall_start_nsec;
  long long avg_nsec = accum_nsec / recv_ctr;
  char buf[1024];
  formatDuration((uint64_t) avg_nsec, buf, 1024, true);
  std::cout << "Avg RTT/request: " <<  buf << std::endl; 
  formatDuration((uint64_t) min_nsec, buf, 1024, true);
  std::cout << "Min RTT/request: " << buf << std::endl;
  std::cout << "Min cyc/request: " << l2aCommas(min_cyc, buf) << std::endl;
  std::cout << "Min reply cycle: " << l2aCommas(reply_cyc, buf) << std::endl;
  formatDuration((uint64_t) overall_nsec, buf, 1024, true);
  std::cout << "Elapsed Time: " << buf << std::endl;
}

void async_send_and_recv(zmq::context_t & ctx, std::ostringstream & ep, int nr_iterations, int64_t bulk_sz) {
  struct drand48_data dr48;
  int result;

  // Setup a random generator to create an identity for a Dealer socket
  result = init_rand(&dr48);
  if (-1 == result) {
    exit(EXIT_FAILURE);
  }

  zmq::socket_t socket (ctx, ZMQ_DEALER);

  char identity[10] = {};
  sprintf(identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));
  socket.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  socket.connect (ep.str());

  do_bulk_async_send_and_recv(socket, nr_iterations, bulk_sz);
}

void async_send(zmq::context_t & ctx, std::ostringstream & ep, int nr_iterations, int64_t bulk_sz) {
  struct drand48_data dr48;
  int result;

  // Setup a random generator to create an identity for a Dealer socket
  result = init_rand(&dr48);
  if (-1 == result) {
    exit(EXIT_FAILURE);
  }

  zmq::socket_t socket (ctx, ZMQ_DEALER);

  char identity[10] = {};
  sprintf(identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));
  socket.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  socket.connect (ep.str());

  do_bulk_async_send(socket, nr_iterations, bulk_sz);
}

void async_recv(zmq::context_t & ctx, uint32_t sv_port_nr, int nr_iterations) {
  std::ostringstream ep;
  ep << "tcp://*:" << sv_port_nr;
  zmq::socket_t socket (ctx, ZMQ_ROUTER);
  socket.bind (ep.str());

  do_bulk_async_recv(socket, nr_iterations);
}

