#include "log.h"
#include "tsc/tsc.h"

extern "C" {
#include "util.h"
}

#include "cxxopts.hpp"

#include <papi.h>
#include <cstdint>
#include <iostream>
#include <iomanip>

void profile_rdtsc(uint64_t iterations);
void profile_rdtscp(uint64_t iterations);

// From https://www.felixcloutier.com/x86/CPUID.html
int main(int argc, char ** argv) {
  uint64_t iter = 10000;
  // Set up logging:
  setLogFileName("/tmp/rtdsc_test");
  toggleConsoleLog(false); // console-logging
  openStatusLogFile();

  cxxopts::Options options("rdtsc_test",
                           "Profiler for rdtsc(p) instructions.");
  options.add_options()
  ("i,iterations", "Number of iterations",
   cxxopts::value<std::string>()->default_value("10000"))
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  options.parse_positional("positional");
  options.parse(argc, argv);

  if (options.count("h") || options.count("positional")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    std::cout << "number_of_iterations accepts postfix modifiers: "
              << std::endl;
    std::cout << "KB, KiB, MB, MiB, GB, GiB" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string iterstring = options["iterations"].as<std::string>();
  iter = convertA2I(iterstring.c_str());


  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT)
    logFail( __FILE__, __LINE__, "PAPI_library_init", retval );

  if (check_rdtsc()) {
    std::cout << "RDTSC instruction supported, profiling rdtsc..." << std::endl;
    profile_rdtsc(iter);
  } else {
    std::cout << "RDTSC instruction not supported" << std::endl;
  }

  if (check_rdtscp()) {
    std::cout << "RDTSCP instruction supported, profiling rdtscp..." << std::endl;
    profile_rdtscp(iter);
  } else {
    std::cout << "RDTSCP instruction not supported" << std::endl;
  }

#if defined(__x86_64__)
 // From https://stackoverflow.com/questions/14783782/which-inline-assembly-code-is-correct-for-rdtscp
  uint64_t rax,rdx;
  uint32_t aux;
  asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
#endif

  closeStatusLogFile();

  return 0;
}

void profile_rdtsc(uint64_t iterations) {
  long long cyc_start, cyc_end, cyc_duration;
  long long usec_start, usec_end, nsec_duration;;

  cyc_start = PAPI_get_real_cyc();
  usec_start = PAPI_get_real_usec();
  for (uint64_t itr = 0; itr < iterations; itr++) {
    //
    // Note: we do not sandwich rdtsc inside cpuid instructions, because we
    // are interested in the rdtsc overhead itself. And out-of-order
    // execution will not get past the surrounding PAPI calls.
    //
#if defined(__x86_64__)
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
#elif defined(__aarch64__)
    uint64_t val;
    asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
#endif
  }
  usec_end = PAPI_get_real_usec();
  cyc_end  = PAPI_get_real_cyc();
  
  cyc_duration = (cyc_end - cyc_start) / iterations;
  nsec_duration = (1000 * (usec_end - usec_start) ) / iterations;

  char buf[1024];
  std::ostringstream msg;
  msg << "Iterations: " << l2aCommas(iterations, buf) << std::endl;
  msg << "nsec/rdtsc: " << l2aCommas(nsec_duration, buf) << std::endl;
  msg << "cyc/rdtsc:  " << l2aCommas(cyc_duration, buf) << std::endl;
  logStatus(__FILE__, __LINE__, msg.str());
  std::cout << "Iterations: " << l2aCommas(iterations, buf) << std::endl;
  std::cout << "nsec/rdtsc: " << l2aCommas(nsec_duration, buf) << std::endl;
  std::cout << "cyc/rdtsc:  " << l2aCommas(cyc_duration, buf) << std::endl;
}

void profile_rdtscp(uint64_t iterations) {
  long long cyc_start, cyc_end, cyc_duration;
  long long usec_start, usec_end, nsec_duration;;

  cyc_start = PAPI_get_real_cyc();
  usec_start = PAPI_get_real_usec();
  for (uint64_t itr = 0; itr < iterations; itr++) {
    //
    // Note: we do not sandwich rdtscp inside cpuid instructions, because we
    // are interested in the rdtscp overhead itself. And out-of-order
    // execution will not get past the surrounding PAPI calls.
    //
#if defined(__x86_64__)
    uint64_t rax,rdx;
    uint32_t aux;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
#elif defined(__aarch64__)
    ;
#endif
  }
  usec_end = PAPI_get_real_usec();
  cyc_end  = PAPI_get_real_cyc();
  
  cyc_duration = (cyc_end - cyc_start) / iterations;
  nsec_duration = (1000 * (usec_end - usec_start) ) / iterations;

  char buf[1024];
  std::ostringstream msg;
  msg << "Iterations:  " << l2aCommas(iterations, buf) << std::endl;
  msg << "nsec/rdtscp: " << l2aCommas(nsec_duration, buf) << std::endl;
  msg << "cyc/rdtscp:  " << l2aCommas(cyc_duration, buf) << std::endl;
  logStatus(__FILE__, __LINE__, msg.str());
  std::cout << "Iterations:  " << l2aCommas(iterations, buf) << std::endl;
  std::cout << "nsec/rdtscp: " << l2aCommas(nsec_duration, buf) << std::endl;
  std::cout << "cyc/rdtscp:  " << l2aCommas(cyc_duration, buf) << std::endl;

}
