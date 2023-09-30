#include "globals.h"
#include "tsc_messages.h"
#include "tsc.h"
#include "log.h"

extern "C" {
#include "util.h"
#include "net.h"
}

#include "cxxopts.hpp"

#include <papi.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <zmq.hpp>

void do_bulk_receive(zmq::socket_t & sock, bool tsc);
// Reply to 'nr_iter' REQ messages. 

void dealer_router_receive(zmq::socket_t & sock, bool tsc);
// Reply to 'nr_iter' DEALER->ROUTER messages.

void async_recv_and_send(zmq::socket_t & recv_sock, zmq::socket_t & send_sock, bool tsc);
// Reply to 'nr_iter' DEALER->ROUTER messages via a separated socket

int main(int argc, char ** argv) {

  // Parse options:
  std::string sv_dotted_quad;
  uint32_t port_number = -1;
  bool async = false; // asynchronous messaging
  bool tsc   = false; // time-stamp each message?
  bool multisocket = false; // use multiple sockets
  bool console_log = false;
  cxxopts::Options options("tsc_slave_2",
                           "cloud_profiler slave 2");
  options.add_options()
  ("sv_ip",           "tsc slave 1 IP address", cxxopts::value<std::string>())
  ("p,port",          "Port number",
   cxxopts::value<uint32_t>(port_number))
  ("a,async",         "Use asynchronous tsc messages (DEALER-ROUTER)",
   cxxopts::value<bool>(async))
  ("m,multi_socket",  "Use multiple sockets",
   cxxopts::value<bool>(multisocket))
  ("c,console",       "Write log-file output to console",
   cxxopts::value<bool>(console_log))
  ("t,tsc",           "Enable TSC time-stamping of reply messages",
   cxxopts::value<bool>(tsc))
  ("h,help",          "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up logging:
  setLogFileName("/tmp/tsc_slave_2");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile();
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  // Set up PAPI:
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT)
    logFail( __FILE__, __LINE__, "PAPI_library_init", retval );
  if (!check_rdtsc()) {
    std::cout << "ERROR: RDTSC instruction not supported, exiting...";
    std::cout << std::endl;
    exit(EXIT_FAILURE);
  }

  if (options.count("sv_ip")) {
    sv_dotted_quad = options["sv_ip"].as<std::string>();
  } else {
    sv_dotted_quad = tsc_master_default_ip;
  }

  zmq::socket_type sock_type;
  if (async) {
    sock_type = (zmq::socket_type)ZMQ_ROUTER;
  } else {
    sock_type = (zmq::socket_type)ZMQ_REP;
  }

  // Construct zmq endpoint:
  std::ostringstream ep;
  if (!options.count("port")) {
    port_number = tsc_master_default_port;
    std::cout << "default port is chosen" << std::endl;
  }
  ep << "tcp://*:" << port_number;
  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t sock_recv (context, sock_type);
  sock_recv.bind (ep.str());

  std::ostringstream ep2;

  if (multisocket) {
    ep2 << "tcp://" << sv_dotted_quad << ":";
    ep2 << port_number;
  }

  if (async && !multisocket) {
    dealer_router_receive(sock_recv, tsc);
  }
  else if (async && multisocket) {
    // Setup a random generator to create an identity for a Dealer socket
    struct drand48_data dr48;
    int result;

    result = init_rand(&dr48);
    if (-1 == result) {
      exit(EXIT_FAILURE);
    }

    // A sending socket for multi-socket Dealer/Router pattern
    zmq::socket_t sock_send (context, ZMQ_DEALER);
    char identity[10] = {};
    sprintf (identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));
    sock_send.setsockopt (ZMQ_IDENTITY, identity, strlen(identity));
    sock_send.connect (ep2.str());

    async_recv_and_send(sock_recv, sock_send, tsc);
  }
  else if (!async && !multisocket) {
    do_bulk_receive(sock_recv, tsc);
  }

  closeStatusLogFile();
  return 0;
}

void do_bulk_receive(zmq::socket_t & sock, bool tsc) {
  int rc;
  uint64_t msg_ctr = 0;
  zmq::message_t msg;

  while (true) {
    rc = sock.recv (&msg);
    int64_t received_ns = PAPI_get_real_nsec();
    int64_t received_cyc = PAPI_get_real_cyc();
    msg_ctr++;
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    // if (msg.size() != sizeof(TSC_msg)) {
    //   std::cout << "Error: message of wrong size received" << std::endl;
    //   std::cout << "Received: " << msg.size() << " expected: " << sizeof(TSC_msg) << std::endl;
    //   std::cout << "Message counter: " << msg_ctr << std::endl;
    //   exit(EXIT_FAILURE);
    // } 
    
    // if (tsc) {
    TSC_msg * m = (TSC_msg *)msg.data();
    // if received shutting down signal -> break loop
    if (m->req_nsec_start == 0) {
      break;
    }
    m->reply_cyc = received_cyc;
    m->reply_nsec_start = received_ns;
    // }
    m->reply_nsec_end = PAPI_get_real_nsec();
    zmq::message_t reply((void *)m, sizeof(TSC_msg));
    rc = sock.send(reply);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket send" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

}

void dealer_router_receive(zmq::socket_t & sock, bool tsc) {
  int rc;
  uint64_t msg_ctr = 0;

  zmq::message_t identity;
  zmq::message_t msg;

  while (true) {
    rc = sock.recv (&identity);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    rc = sock.recv (&msg);
    msg_ctr++;
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (msg.size() != sizeof(TSC_msg)) {
      std::cout << "Error: message of wrong size received" << std::endl;
      std::cout << "Received: " << msg.size() << " expected: " << sizeof(TSC_msg) << std::endl;
      std::cout << "Message counter: " << msg_ctr << std::endl;
      exit(EXIT_FAILURE);
    } 
    rc = sock.send(identity, ZMQ_SNDMORE);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket send" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (tsc) {
      TSC_msg * m = (TSC_msg *)msg.data();
      m->reply_cyc = PAPI_get_real_cyc();
    }
    rc = sock.send(msg);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket send" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

}

void async_recv_and_send(zmq::socket_t & recv_sock, zmq::socket_t & send_sock, bool tsc) {
  int rc;
  uint64_t msg_ctr = 0;

  zmq::message_t identity;
  zmq::message_t msg;

  while (true) {
    rc = recv_sock.recv (&identity);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    rc = recv_sock.recv (&msg);
    msg_ctr++;
    if (rc < 0) {
      std::cout << "Error in ZMQ socket receive" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (msg.size() != sizeof(TSC_msg)) {
      std::cout << "Error: message of wrong size received" << std::endl;
      std::cout << "Received: " << msg.size() << " expected: " << sizeof(TSC_msg) << std::endl;
      std::cout << "Message counter: " << msg_ctr << std::endl;
      exit(EXIT_FAILURE);
    } 
    if (tsc) {
      TSC_msg * m = (TSC_msg *)msg.data();
      m->reply_cyc = PAPI_get_real_cyc();
    }
    rc = send_sock.send (msg);
    if (rc < 0) {
      std::cout << "Error in ZMQ socket send" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
