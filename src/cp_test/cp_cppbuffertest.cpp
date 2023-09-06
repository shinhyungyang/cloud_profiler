#include "time_tsc.h" // please refer this file.
#include "cloud_profiler.h"
#include "globals.h"
extern "C" {
#include "util.h"
#include "net.h"
}
#include "cxxopts.hpp"

#include <unistd.h>
#include <stdlib.h>
//#include <time.h>
#include <assert.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> terminate(false);

void thread_f_timer(int nr_seconds);
void thread_f (int tid, int nr_iterations, int ptest_duration, bool netcnf);

void printHeader(std::string msg);

void printResults(
  std::string msg,
  timespec    start,
  timespec    end,
  int         nr_iterations);
  bool        netcnf = false;

int main(int argc, char ** argv) {
  int nr_threads = 1;
  int ptest_duration = 5;
  cxxopts::Options options("cp_cpptest",
                           "C++ test of the cloud_profiler API.");
  options.add_options()
  ("i,iterations", "Number of iterations",
   cxxopts::value<std::string>()->default_value("1MB"))
  ("t,threads", "Number of threads",
   cxxopts::value<int>(nr_threads))
  ("d,duration", "Duration of periodic counter test",
   cxxopts::value<int>(ptest_duration))
  ("n,netconf", "Use configuration server")
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  options.parse_positional("positional");
  auto option_result = options.parse(argc, argv);

  if (option_result.count("h") || option_result.count("positional")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    std::cout << "number_of_iterations accepts postfix modifiers: "
              << std::endl;
    std::cout << "KB, KiB, MB, MiB, GB, GiB" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string iterstring = option_result["iterations"].as<std::string>();
  int nr_iterations = convertA2I(iterstring.c_str());

  if (option_result.count("n")) {
    netcnf = true;
  }

  if (ptest_duration < 5 ) {
    std::cout << "WARNING: some testcases may exceed the specified test duration of ";
    std::cout << ptest_duration << " seconds!" << std::endl;
  }

  // Determine our IP address:
  char tmpAddr[255];
  if (0 == getIPDottedQuad(tmpAddr, 255)) {
    std::cout << "IP address: " << tmpAddr << std::endl;
  } else {
    std::cout << "Could not determine IP address." << std::endl;
  }

  // Set number of threads
  //nr_threads = 2;

  // Multi-threaded tests:
  std::thread tids[nr_threads];
  for (int i = 0; i<nr_threads; i++) {
    tids[i] = std::thread(thread_f, i, nr_iterations, ptest_duration, netcnf);
  }
  for (int i = 0; i<nr_threads; i++) {
    tids[i].join();
  }

  //std::cout << "main wait for 5 seconds" << std::endl;
  //std::this_thread::sleep_for(std::chrono::seconds(5));
  return 0;
}

void thread_f (int tid, int nr_iterations, int ptest_duration, bool netcnf) {
  int64_t ch;

  // ID HANDLER
  //ch = openChannel("identity          ", ASCII_TSC, IDENTITY);
  // BUFFER ID HANDLER
  //ch = openChannel("buffer_identity   ", ASCII, BUFFER_IDENTITY);
  // BUFFER ID HANDLER WITH COMPRESSION
  ch = openChannel("buffer_identity_comp", ZSTD, BUFFER_IDENTITY_COMP);
  // Null Handler
  //ch = openChannel("Null   ", ASCII, NULLH);

  //
  // Log timestamps:
  //
  timespec start, end;

  getTS(&start);
  //std::cout << nr_iterations << std::endl;
  for (int i = 0; i < nr_iterations; i++) {
    logTS(ch, i);
  }
  getTS(&end);

  printResults("logTS", start, end, nr_iterations);
  //
  // Close channels:
  //
  closeChannel(ch);
}

void printResults(std::string msg,
                  timespec    start,
                  timespec    end,
                  int         nr_iterations)
{
  struct timespec diff, quotient;
  // Evaluate and print results:
  printHeader(msg);
  subtractTS(&diff, &start, &end);
  divideTS(&quotient, &diff, nr_iterations);
  char buf[1024];
  formatTS(&start, buf, 1024);
  std::cout << "Start:     " << buf << std::endl;
  formatTS(&end, buf, 1024);
  std::cout << "End:       " << buf << std::endl;
  formatDuration(&diff, buf, 1024, true);
  std::cout << "Duration:  " << buf << std::endl;
  std::cout << "Calls:     " << l2aCommas(nr_iterations, buf) << std::endl;
  formatDuration(&quotient, buf, 1024, true);
  std::cout << "time/call: " << buf << std::endl;
}

void printHeader(std::string msg) {
  size_t len = msg.length();
  for (size_t c = 0; c <= len; c++) {
    std::cout << "=";
  }
  std::cout << std::endl << msg << ":" << std::endl;
  for (size_t c = 0; c <= len; c++) {
    std::cout << "=";
  }
  std::cout << std::endl;
}
