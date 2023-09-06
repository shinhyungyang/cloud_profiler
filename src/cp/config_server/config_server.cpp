#include "globals.h"
#include "log.h"
#include "net_conf.h"
#include "parser.h"
#include "fox/fox_config_server.h"

#include "cxxopts.hpp"
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <zmq.hpp>

int main(int argc, char ** argv) {
  // Parse options:
  int32_t port_number = -1;
  bool console_log = false;
  cxxopts::Options options("config_server",
                           "cloud_profiler configuration server");
  options.add_options()
  ("cnf", "configuration table name", cxxopts::value<std::string>()) 
  ("c,console", "Write log-file output to console",
   cxxopts::value<bool>(console_log))
  ("p,port", "Port number",
   cxxopts::value<int32_t>(port_number))
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  options.parse_positional("positional");
  auto option_result = options.parse(argc, argv);

  if (option_result.count("h") || option_result.count("positional")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up logging:
  setLogFileName("/tmp/conf_server");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile();
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  // Select configuration table:
  if (option_result.count("cnf")) {
    // User requested particular configuration table:
    std::string cnf = option_result["cnf"].as<std::string>();
    setConfigTable(cnf);
  } else {
    setDefaultConfigTable();
  }

  // Construct zmq endpoint:
  std::ostringstream ep;
  if (port_number == -1) {
    port_number = conf_server_default_port;
  }
  ep << "tcp://*:" << port_number;
  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind (ep.str());

  // Process client requests:
  while (true) {
    zmq::message_t request;

    //  Wait for client request:
    socket.recv (&request);
    conf_req_msg * msg = (conf_req_msg *) request.data();

    // Log the request:
    logStatus(__FILE__, __LINE__, msg->toStr());

    // Look up the configuration for the requested channel:
    conf_reply_msg reply_msg;
    uint32_t line_nr;
    bool ret = lookupConfig(msg, &reply_msg, &line_nr);
    const auto& errorCode = reply_msg.err;
    if (!ret) {
      // Configuration cannot be obtained. 'reply_msg' already contains the
      // error value so the client will find out about it.
      // Here we only enter the incident in the log.

      // not found
      if (errorCode == ERR_CONF_NOT_FOUND) 
      {
          logStatus(__FILE__, __LINE__, "Configuration cannot be obtained (no matching entry for request)");
      }
      // found, but error related fox handler
      else if (reply_msg.h_type == handler_type::FOX_START || reply_msg.h_type == handler_type::FOX_END)
      {
        logStatus(__FILE__, __LINE__, getErrorMsgFox(errorCode));
      }
      //
      else 
      {
        logStatus(__FILE__, __LINE__, 
          std::string("unknown error, msg->err returned: " + std::to_string(reply_msg.err)));
      }
    } else {
      // Configuration obtained, log reply:
      std::ostringstream log_reply;
      log_reply << "Reply (match in 0-indexed entry # " << line_nr << "): ";
      log_reply << reply_msg.toStr();
      // if (msg->ch_name == std::string("fox_start"))
      // {
      //   log_reply << "start node ip address: " << msg->par_value[1] << ".";
      //   log_reply << msg->par_value[2] << "." << msg->par_value[3] << ".";
      //   log_reply << msg->par_value[4] << ":" << msg->par_value[5] << " | ";
      // }
      logStatus(__FILE__, __LINE__, log_reply.str());
    }

    // Send reply to client:
    zmq::message_t reply ((void *) &reply_msg, sizeof(conf_reply_msg));
    socket.send (reply);
    // When multiple starting nodes request fox_start handler, terminate
    if (errorCode == ERR_FOX_MULTIPE_START_NODE) {
      logStatus(__FILE__, __LINE__, std::string("Terminating Config Server due to multiple starting nodes"));
      break;
    }
  }
  closeStatusLogFile();
  return 0;
}

  /*
  file_iterator<> first("channels.cfg");
  if (!first) {
    std::cout << "Unable to open configuration file!\n";
    return -1;
  }
  file_iterator<> last = first.make_end();

  std::cout << parse_numbers(first, last);
  */
