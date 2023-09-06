#include "time_tsc.h"
#include "cloud_profiler.h"
#include "globals.h"

extern "C" {
#include "util.h"
}

#include "cxxopts.hpp"

#include <iostream>
#include <sys/types.h>
#include <time.h>
#include <cstdint>

int main (int argc, char ** argv) {

  struct timespec ts_wait, ts_now, ts_sleep;
  uint32_t  interval = 1000 * 1000; // defaults to emit per a millisecond

  // parse command-line options:
  cxxopts::Options options("FOX_END client",
                           "A C++11 mockup client to test FOX_END handler");
  options.add_options()
  ("i,iterations",  "Number of iterations",
   cxxopts::value<std::string>()->default_value("1MB"))
  ("n,interval",    "Nanoseconds to pause between tuple emissions",
   cxxopts::value<uint32_t>(interval))
  ("h,help", "Usage information")
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

  uint64_t periodic_inc_ns = interval;
  struct timespec periodic_inc;
  periodic_inc.tv_sec   = periodic_inc_ns / NANO_S_PER_S;
  periodic_inc.tv_nsec  = periodic_inc_ns % NANO_S_PER_S;

  int64_t ch0 = openChannel("fox_end", ASCII, NET_CONF);
  int64_t ch1 = openChannel("ODROIDN2   GPIORTT", ASCII, NET_CONF);

  if (!(isOpen(ch0) && isOpen(ch1))) {
    std::cerr << "Channel failed to open." << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set the first completion timestamp
  clock_gettime_tsc(CLOCK_REALTIME, &ts_wait);
  ts_wait.tv_sec += periodic_inc.tv_sec;
  ts_wait.tv_nsec += periodic_inc.tv_nsec;

  bool doSnooze;
  bool skippedFirst = false;
  for (long iter = 0; iter <= nr_iterations; iter++) {
    if (skippedFirst == true) {
      logTS(ch0, iter);
      logTS(ch1, iter);
    } else {
      skippedFirst = true;
    }
    doSnooze = true;
    while (doSnooze && (iter <= nr_iterations)) {
      clock_gettime_tsc(CLOCK_REALTIME, &ts_now);
      if (compareTS(&ts_now, &ts_wait) > 0) {
        addTS(&ts_wait, &ts_wait, &periodic_inc);
        doSnooze = false;
      } else {
        subtractTS(&ts_sleep, &ts_now, &ts_wait);
        if (compareTS(&ts_sleep, &periodic_inc) > 0) {
          ts_sleep.tv_sec   = periodic_inc.tv_sec;
          ts_sleep.tv_nsec  = periodic_inc.tv_nsec;
        }
        nanosleep(&ts_sleep, NULL);
      }
    } // while (sleeping)
  }

  return 0;
}
