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
void thread_f (int tid, int ptest_duration, bool netcnf);

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
  options.parse(argc, argv);

  if (options.count("h") || options.count("positional")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    std::cout << "number_of_iterations accepts postfix modifiers: "
              << std::endl;
    std::cout << "KB, KiB, MB, MiB, GB, GiB" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string iterstring = options["iterations"].as<std::string>();
  int nr_iterations = convertA2I(iterstring.c_str());

  if (options.count("n")) {
    netcnf = true;
  }

  if (ptest_duration < 5 ) {
    std::cout << "WARNING: some testcases may exceed the specified test duration of ";
    std::cout << ptest_duration << " seconds!" << std::endl;
  }

  timespec start, end;

  //
  // Run getTS() in tight loop:
  //
  timespec ts;
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    getTS(&ts);
  }
  getTS(&end);
  printResults("getTS", start, end, nr_iterations);


  //
  // Run logTS Buffer ID handler (ASCII) in tight loop:
  //
  int64_t ch_idh_buffer = openChannel("ch_test_id_buffer", ASCII,
                                                        BUFFER_IDENTITY);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    logTS(ch_idh_buffer, iter);
  }
  getTS(&end);
  printResults("logTS Buffer ID handler", start, end, nr_iterations);
  closeChannel(ch_idh_buffer);

  //
  // Run logTS Buffer ID handler with compression in tight loop:
  //
  int64_t ch_idh_buffer_bin = openChannel("ch_test_id_buffer_bin", ZSTD,
                                                        BUFFER_IDENTITY_COMP);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    logTS(ch_idh_buffer_bin, iter);
  }
  getTS(&end);
  printResults("logTS Binary Buffer ID handler with compression",
                                               start, end, nr_iterations);
  closeChannel(ch_idh_buffer_bin);

  //
  // Run logTS ID handler in tight loop:
  //
  int64_t ch_idh = openChannel("ch_test_id", ASCII, IDENTITY);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    logTS(ch_idh, iter);
  }
  getTS(&end);
  printResults("logTS ID handler", start, end, nr_iterations);

  //
  // Run logTS ID handler in tight loop:
  //
  int64_t ch_idh_binary = openChannel("ch_test_id_bin", BINARY, IDENTITY);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    logTS(ch_idh_binary, iter);
  }
  getTS(&end);
  printResults("logTS Binary ID handler", start, end, nr_iterations);

  //
  // Run logTS null handler in tight loop:
  //
  int64_t ch_nullh = openChannel("ch_test_null", ASCII, NULLH);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    logTS(ch_nullh, iter);
  }
  getTS(&end);
  printResults("logTS NULL handler", start, end, nr_iterations);

  //
  // Run nologTS null handler in tight loop:
  //
  int64_t ch_nl = openChannel("ch_test_nolog", ASCII, NULLH);
  //FIXME:
  //int64_t ch_nl = openChannel("ch_test_nolog", ASCII, NET_CONF);
  getTS(&start);
  for (long iter = 0; iter < nr_iterations; iter++) {
    nologTS(ch_nl, iter);
  }
  getTS(&end);
  printResults("nologTS NULL handler", start, end, nr_iterations);


  // Determine our IP address:
  char tmpAddr[255];
  if (0 == getIPDottedQuad(tmpAddr, 255)) {
    std::cout << "IP address: " << tmpAddr << std::endl;
  } else {
    std::cout << "Could not determine IP address." << std::endl;
  } 

  // Multi-threaded tests:
  std::thread tids[nr_threads];
  for (int i = 0; i<nr_threads; i++) {
    tids[i] = std::thread(thread_f, i, ptest_duration, netcnf);
  }
  for (int i = 0; i<nr_threads; i++) {
    tids[i].join();
  }

  return 0;
}

void thread_f (int tid, int ptest_duration, bool netcnf) {
  int64_t ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch10;
  //
  // Open & parameterize channels:
  //
  if (netcnf) {
      // Configuration from the server:
      ch0 = openChannel("identity          ", ASCII, NET_CONF);
      ch1 = openChannel("downsample       2", ASCII, NET_CONF);
      ch2 = openChannel("downsample      10", ASCII, NET_CONF);
      ch3 = openChannel("XoY 100      16384", ASCII, NET_CONF);
      ch4 = openChannel("XoY 1000     32768", ASCII, NET_CONF);
      ch5 = openChannel("XoY 2         1KiB", ASCII, NET_CONF);
      ch6 = openChannel("FirstLast      all", ASCII, NET_CONF);
      ch7 = openChannel("FirstLast        0", ASCII, NET_CONF);
      ch8 = openChannel("FirstLast        1", ASCII, NET_CONF);
      ch10 = openChannel("buffer_identity   ", ASCII, NET_CONF);
      if (!(isOpen(ch0) && isOpen(ch1) && isOpen(ch2) && isOpen(ch3) &&
            isOpen(ch4) && isOpen(ch5) && isOpen(ch6) && isOpen(ch7) &&
            isOpen(ch8) && isOpen(ch10))) {
        std::cout << "Warning: some channel(s) failed to open. ";
        std::cout << "Check the cloud_profiler log file." << std::endl;
      }
  } else {
    // Programmatic configuration:
    ch0 = openChannel("identity          ", ASCII, IDENTITY);
    ch1 = openChannel("downsample       2", ASCII, DOWNSAMPLE);
    ch2 = openChannel("downsample      10", ASCII, DOWNSAMPLE);
    ch3 = openChannel("XoY 100      16384", ASCII, XOY);
    ch4 = openChannel("XoY 1000     32768", ASCII, XOY);
    ch5 = openChannel("XoY 2         1KiB", ASCII, XOY);
    ch6 = openChannel("FirstLast      all", ASCII, FIRSTLAST);
    ch7 = openChannel("FirstLast        0", ASCII, FIRSTLAST);
    ch8 = openChannel("FirstLast        1", ASCII, FIRSTLAST);
    ch10 = openChannel("buffer_identity   ", ASCII, BUFFER_IDENTITY);
    if (!(isOpen(ch0) && isOpen(ch1) && isOpen(ch2) && isOpen(ch3) &&
          isOpen(ch4) && isOpen(ch5) && isOpen(ch6) && isOpen(ch7) &&
          isOpen(ch8) && isOpen(ch10))) {
      std::cout << "Warning: some channel(s) failed to open. ";
      std::cout << "Check the cloud_profiler log file." << std::endl;
    }
    if (-1 == parameterizeChannel(ch1, 0, 2)) {
      std::cout << "Could not parameterize channel" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (-1 == parameterizeChannel(ch2, 0, 10)) {
      std::cout << "Could not parameterize channel 2" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (-1 == parameterizeChannel(ch4, 1, 32768)) {
      std::cout << "Could not parameterize channel 4" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (-1 == parameterizeChannel(ch4, 0, 1000)) {
      std::cout << "Could not parameterize channel 4" << std::endl;
      exit(EXIT_FAILURE);
    }
    // Note "KiB" for 1024 and "KB" for 1000:
    if (-1 == parameterizeChannel(ch5, 1, "1KiB")) {
      std::cout << "Could not parameterize channel 5, parm 1" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (-1 == parameterizeChannel(ch5, 0, 2)) {
      std::cout << "Could not parameterize channel 5, parm 0" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  std::ostringstream name_strm;
  name_strm << "PeriodicCounter_" << tid;
  int64_t ch9 = openChannel(name_strm.str(), ASCII, FASTPERIODICCOUNTER);
  //int64_t ch9 = openChannel(name_strm.str(), ASCII, PERIODICCOUNTER);
  
  //
  // Log timestamps:
  //
  for (int i = 0; i < 200000; i++) {
    logTS(ch3, i);
    logTS(ch4, i);
    logTS(ch5, i);
    logTS(ch6, i);
    logTS(ch10, i);
  }
  logTS(ch8, 0);
  logTS(ch3, (int64_t) -1);
  logTS(ch4, (int64_t) -1);
  logTS(ch5, (int64_t) -1);
  logTS(ch6, (int64_t) -1);
  logTS(ch7, (int64_t) -1);
  logTS(ch8, (int64_t) -1);
  logTS(ch10, (int64_t) -1);

  for (int i = 0;i<=20;i++) {
    logTS(ch0, (int64_t) i);
    logTS(ch1, (int64_t) i);
    logTS(ch2, (int64_t) i);
  }

  //
  // Close channels:
  //
  logTS(ch0, (int64_t) -1); // Poison pill
  logTS(ch1, (int64_t) -1); // Poison pill
  closeChannel(ch1);        // Test double-close of channel.
  closeChannel(ch2);
  for (int i = 14000;i<18000;i++) {
    logTS(ch0, (int64_t) i); // Has no effect after poison pill closed
    logTS(ch1, (int64_t) i); // the channel.
    logTS(ch2, (int64_t) i);
  }

  //
  // PeriodicCounter test:
  //
  std::thread t_timer;
  if (tid == 0) {
    // Announce the test:
    std::ostringstream msg_strm;
    msg_strm << "PeriodicCounter test for " << ptest_duration << " seconds";
    printHeader(msg_strm.str());
    // Start the timer thread:
    t_timer = std::thread(thread_f_timer, ptest_duration);
  }
  // Log periodic counts:
  switch (tid) {
    case 1:
      // Thread 1:
      while (terminate == false) {
        std::this_thread::sleep_for (std::chrono::milliseconds(300));
        logTS(ch9, 0);
      }
      break;
    case 2: {
        // Thread 2:
        //
        // Log 60 events @ 20 events/s
        // Sleep for 3 seconds
        // Repeat: log at 20 events/s 
        int round = 1;
        while (terminate == false) {
          std::this_thread::sleep_for (std::chrono::milliseconds(50));
          logTS(ch9, 0);
          if (round == 60) {
            std::this_thread::sleep_for (std::chrono::seconds(3));
          }
          round++;
        }
      }
      break;
    case 3: {
        // Thread 3:
        //
        // Log 61 events at 20 events/s;
        // Set counter period to 500ms;
        // Repeat: sleep (1ms <= x <= 200ms) -> log;
        int round = 0;
        while (terminate == false && round <= 60) {
          std::this_thread::sleep_for (std::chrono::milliseconds(50));
          logTS(ch9, 0);
          if (round == 60) {
            // Reset period to 500ms:
            if (-1 == parameterizeChannel(ch9, 0, (int64_t)(NANO_S_PER_S / 2))) {
              std::cout << "Could not parameterize channel" << std::endl;
              exit(EXIT_FAILURE);
            }
          }
          round++;
        }
        while (terminate == false) {
          uint16_t sl = 1 + rand() % 200;
          std::this_thread::sleep_for (std::chrono::milliseconds(sl));
          logTS(ch9, 0);
        }
      }
      break;
    case 4: {
        // Thread 4:
        //
        // Log 15 events at 10 events/s;
        // Sleep for 2 s
        // Set counter period to 500ms;
        // Repeat: log at 10 events/s;
        int round = 1;
        while (terminate == false) {
          std::this_thread::sleep_for (std::chrono::milliseconds(100));
          logTS(ch9, 0);
          if (round == 15) {
            std::this_thread::sleep_for (std::chrono::seconds(2));
            // Reset period to 500ms:
            if (-1 == parameterizeChannel(ch9, 0, (int64_t)(NANO_S_PER_S / 2))) {
              std::cout << "Could not parameterize channel" << std::endl;
              exit(EXIT_FAILURE);
            }
          }
          round++;
        }
      }
      break;
    case 5: {
        // Thread 5:
        //
        // Log once and sleep;
        bool logged_once = false;
        while (terminate == false) {
          if (!logged_once) {
            logTS(ch9, 0);
            logged_once = true;
          }
        }
      }
      break;
    case 6:
        // Thread 6:
        //
        // Never log anything:
        while (terminate == false) { ; }
      break;
    case 7:
      // Thread 7:
      //
      // Sleep for two seconds before continuously logging:
      std::this_thread::sleep_for (std::chrono::seconds(2));
      while (terminate == false) {
        logTS(ch9, 0);
      }
      break;
    case 8:
      // Thread 8:
      //
      // Sleep for two seconds;
      // Repeat: log 1 s @ 10 events/s, sleep 1 s;
      // Logging: log->sleep
      std::this_thread::sleep_for (std::chrono::seconds(2));
      while (terminate == false) {
        int round = 0;
        while(!terminate && round < 10) {
          logTS(ch9, 0);
          std::this_thread::sleep_for (std::chrono::milliseconds(100));
          round++;
        }
        if (!terminate) {
          std::this_thread::sleep_for (std::chrono::seconds(1));
        }
      }
      break;
    case 9:
      // Thread 9:
      //
      // Sleep for two seconds;
      // Repeat: log 1 s @ 10 events/s, sleep 1 s;
      // Logging: sleep->log
      std::this_thread::sleep_for (std::chrono::seconds(2));
      while (terminate == false) {
        int round = 0;
        while(!terminate && round < 10) {
          std::this_thread::sleep_for (std::chrono::milliseconds(100));
          logTS(ch9, 0);
          round++;
        }
        if (!terminate) {
          std::this_thread::sleep_for (std::chrono::seconds(1));
        }
      }
      break;
    case 10:
      // Thread 10:
      //
      // Sleep 100ms;
      // Set counter period to 100ms;
      // Sleep 1s;
      // Repeat: log @ 10 events/s;
      std::this_thread::sleep_for (std::chrono::milliseconds(100));
      if (-1 == parameterizeChannel(ch9, 0, (int64_t)(NANO_S_PER_S / 10))) {
        std::cout << "Could not parameterize channel" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (terminate == false) {
        std::this_thread::sleep_for (std::chrono::seconds(1));
      }
      while (terminate == false) {
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
        logTS(ch9, 0);
      }
      break;
    case 11: {
        // Thread 11:
        //
        // Sleep for 2s;
        // Set period to 2s;
        // Repeat: sleep
        if (!terminate) {
          std::this_thread::sleep_for (std::chrono::seconds(2));
        }
        if (!terminate) {
          if (-1 == parameterizeChannel(ch9, 0, (int64_t)(2 * NANO_S_PER_S))) {
            std::cout << "Could not parameterize channel" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
        while (terminate == false) {
          std::this_thread::sleep_for (std::chrono::milliseconds(100));
        }
      }
      break;
    case 12: {
        // Thread 12:
        //
        // Repeat:
        //   sleep (1 < x < 1500ms);
        //   Decrement counter period by 20 ms;
        uint64_t cp   = NANO_S_PER_S;
        uint64_t decr = (NANO_S_PER_S / 1000) * 20;
        while (terminate == false) {
          uint16_t sl = 1 + rand() % 1500;
          std::this_thread::sleep_for (std::chrono::milliseconds(sl));
          cp -=  decr;
          if (-1 == parameterizeChannel(ch9, 0, cp)) {
            std::cout << "Could not parameterize channel" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
      }
      break;
    case 13: {
        // Thread 13:
        //
        // Count the total number of events, by setting the period
        // to 10 days.
        if (-1 == parameterizeChannel(ch9, 0, 10*NANO_S_PER_DAY)) {
          std::cout << "Could not parameterize channel" << std::endl;
          exit(EXIT_FAILURE);
        }
        while (terminate == false) {
          logTS(ch9, 0);
        }
      }
      break;
    default:
      // Thread 0 and others:
      while (terminate == false) {
        logTS(ch9, 0);
      }
  }
  logTS(ch9, (int64_t) -1); // Poison pill
  if (tid == 0) {
    t_timer.join();
  }
}

void thread_f_timer(int nr_seconds) {
  assert(nr_seconds > 0 && "Timer thread: invalid argument");
  for (int secs = 0; secs < nr_seconds; secs++) {
    std::cout << ".";
    std::cout.flush();
    std::this_thread::sleep_for (std::chrono::seconds(1));
  }
  std::cout << std::endl;
  terminate = true;
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
