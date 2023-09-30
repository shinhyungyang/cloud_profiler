#include "cloud_profiler.h"
#include "globals.h"
#include "cxxopts.hpp"

#include <unistd.h>
#include <iostream>

void thread_f (int tid, bool netcnf);

bool netcnf   = false;
bool isMaster = false;

int main(int argc, char ** argv) {
  cxxopts::Options options("cp_gpiotest",
                           "Test of the GPIO master-slave system.");
  options.add_options()
  ("n,netconf", "Use configuration server")
  ("g,gpio", "GPIO test")
  ("m,master", "GPIO test: server-side")
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  options.parse_positional("positional");
  options.parse(argc, argv);

  if (options.count("h") || options.count("positional")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  if (options.count("n")) {
    netcnf = true;
  }

  if (options.count("g")) {

    if (options.count("m")) {
      isMaster = true;
    }
  }

  int64_t ch0;
  //
  // Open a GPIORTT channel
  //
  if (netcnf) {
    ch0 = openChannel("ODROIDN2   GPIORTT", ASCII, NET_CONF);
    if (!(isOpen(ch0))) {
      std::cout << "Warning: some channel(s) failed to open. ";
      std::cout << "Check the cloud_profiler log file." << std::endl;
    }
  } else {
    ch0 = openChannel("ODROIDN2   GPIORTT", ASCII, GPIORTT);
    if (!(isOpen(ch0))) {
      std::cout << "Warning: some channel(s) failed to open. ";
      std::cout << "Check the cloud_profiler log file." << std::endl;
    }
  }

  if (isMaster) {
    sleep(60);
  } else {
    for (int i = 0; i < 1; i++) {
      logTS(ch0, i);
    }
  }

  closeChannel(ch0);  // Close the GPIORTT channel

  return 0;
}

