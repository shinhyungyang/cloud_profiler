#include "globals.h"
#include "handlers.h"
#include "log.h"
#include "net_conf.h"

extern "C" {
#include "net.h"
}

#include "cxxopts.hpp"

#include <zmq.hpp>
#include <string>
#include <sstream>
#include <iostream>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
}

int main (int argc, char ** argv) {
  // configuration server settings:
  std::string sv_dotted_quad;
  uint32_t sv_port_nr;
  // channel (configuration client) settings:
  std::string ch_name, ch_dotted_quad;
  uint32_t ch_nr;
  uint32_t ch_pid;
  uint32_t ch_tid;
  // logging to console setting:
  bool console_log = false;
  // parse command-line options:
  cxxopts::Options options("config_client",
                           "cloud_profiler configuration client");
  options.add_options()
  ("ch_name", "channel name", cxxopts::value<std::string>())
  ("ch_ip",   "channel ip address", cxxopts::value<std::string>())
  ("ch_pid",  "channel pid", cxxopts::value<uint32_t>(ch_pid))
  ("ch_tid",  "channel tid", cxxopts::value<uint32_t>(ch_tid))
  ("ch_nr",   "channel number", cxxopts::value<uint32_t>(ch_nr))
  ("sv_ip",   "server IP address", cxxopts::value<std::string>())
  ("sv_p",    "server port number",
   cxxopts::value<uint32_t>(sv_port_nr))
  ("c,console", "Enagle log-file output to console",
   cxxopts::value<bool>(console_log))
  ("h,help", "Usage information")
  ;

  auto option_result = options.parse(argc, argv);

  if (option_result.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  setLogFileName("/tmp/conf_client");
  toggleConsoleLog(console_log); // console-logging
  openStatusLogFile(); 

  // Set up data to request configuration:

  if (option_result.count("sv_ip")) {
    sv_dotted_quad = option_result["sv_ip"].as<std::string>();
  } else {
    sv_dotted_quad = conf_server_default_ip;
  }
  if (!option_result.count("sv_p")) {
    sv_port_nr = conf_server_default_port;
  }
  if (option_result.count("ch_name")) {
    ch_name = option_result["ch_name"].as<std::string>();
  } else {
    ch_name = "default_channel";
  }
  if (option_result.count("ch_ip")) {
    ch_dotted_quad = option_result["ch_ip"].as<std::string>(); 
  } else {
    char tmp_buf[MAX_STR_SIZE];
    if (0 == getIPDottedQuad(tmp_buf, MAX_STR_SIZE)) {
      ch_dotted_quad = tmp_buf;
    } else {
      ch_dotted_quad = "NO.IP.ADDR";
      logFail(__FILE__, __LINE__, "unable to obtain IP address");
    }
  }
  if (!option_result.count("ch_pid")) {
    ch_pid = getpid();
  }
  if (!option_result.count("ch_tid")) {
    ch_tid = syscall(SYS_gettid);
  }
  if (!option_result.count("ch_nr")) {
    ch_nr = -1;
  }

  handler_type h;
  log_format log_f;
  int64_t par[conf_server_max_param];
  getConfigFromServer(sv_dotted_quad, sv_port_nr, ch_name, ch_dotted_quad,
		      ch_pid, ch_tid, ch_nr, &h, par, &log_f);
  return 0;
}
