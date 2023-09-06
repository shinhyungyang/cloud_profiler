#define __USE_POSIX // for localtime_r
#define _POSIX_C_SOURCE 199309

#include "time_tsc.h"

#include "log.h"

extern "C" {
#include "util.h"
}

#include "cloud_profiler.h"
#include "cxxopts.hpp"

#include <papi.h>
#include <cstdint>
#include <iostream>

int main(int argc, char ** argv) {
  // Set up logging:
  setLogFileName("/tmp/cp_tstest");
  toggleConsoleLog(false); // console-logging
  openStatusLogFile();

  cxxopts::Options options("cp_tstest",
                           "Test of redefined timespec and clock_gettime_tsc");
  options.add_options()
  ("h,help", "Usage information")
  ;

  options.parse_positional("positional");
  auto option_result = options.parse(argc, argv);

  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT)
    logFail( __FILE__, __LINE__, "PAPI_library_init", retval );

  // do test
  char buf[1024];
  struct timespec ts;
  if (0 != clock_gettime_tsc(CLOCK_REALTIME, &ts))
    logFail(__FILE__, __LINE__, "clock_gettime_tsc", retval);
  formatTS(&ts, buf, 1024);
  std::cout << "ts: " << buf << " (" << ts.tsc << ")" << std::endl;

  closeStatusLogFile();

  return 0;
}

