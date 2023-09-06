#include "cloud_profiler.h"

#include "cxxopts.hpp"

#include <iostream>
#include <sys/types.h>
#include <signal.h>

int main (int argc, char ** argv) {

  // parse command-line options:
  cxxopts::Options options("FOX_START client",
                           "A C++11 mockup client to test FOX_START");
  options.add_options()
  ("h,help", "Usage information")
  ;

  auto option_result = options.parse(argc, argv);

  if (option_result.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  int64_t ch0 = openChannel("fox_start", ASCII, NET_CONF);
  int64_t ch1 = openChannel("ODROIDN2   GPIORTT", ASCII, NET_CONF);

  if (!(isOpen(ch0) && isOpen(ch1))) {
    std::cerr << "Channel failed to open." << std::endl;
    exit(EXIT_FAILURE);
  }

  sigset_t mask;
  sigemptyset(&mask);
  sigsuspend(&mask);

  return 0;
}
