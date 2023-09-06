#include "time_tsc.h"

extern "C" {
#include "net.h"
}

#include "globals.h"
#include "cloud_profiler.h"
#include "log.h"
#include "exec.h"

#include <cstdlib>
#include <cstdint>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <iterator>
#include <algorithm>
#include <vector>
#include <iomanip>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
}

#include "cxxopts.hpp"

struct clock_error {
  double system_time_offset;
  double root_delay;
  double root_dispersion;
};

static void run_interval_of(uint64_t nsec);
static bool shallTerminate();
static void set_log_file_as(std::ofstream & logf, const char * name);
static void print_clock_err(std::ostream  & outf, struct timespec * ts,
                            struct clock_error * err);

std::ofstream ntplogd_logf;

// Arguments to read from users
bool      console_log;
uint32_t  interval;
uint32_t  iteration;
bool      periodic;       // run the program infinitely?
bool      nonstop;

int main(int argc, char ** argv) {
  // Parse options:
  // TODO: "-c 10 -p"
  console_log = false;
  interval = 1;
  iteration = 0;
  periodic = false;
  nonstop = false;

  cxxopts::Options options("ntplogd",
                           "cloud_profiler NTP clock error logging daemon");
  options.add_options()
  ("c,console",     "Write log-file output to console",
   cxxopts::value<bool>(console_log))
  ("p,periodic",    "Periodically do measurements",
   cxxopts::value<bool>(periodic))
  ("n,interval",    "The number of seconds to pause between measurements",
   cxxopts::value<uint32_t>(interval))
  ("i,iteration",   "The number of measurements to do",
   cxxopts::value<uint32_t>(iteration))
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  auto option_result = options.parse(argc, argv);

  if (option_result.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set Console log
  setLogFileName("/tmp/ntplogd_logfile");
  toggleConsoleLog(console_log);
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  // Set a log stream
  set_log_file_as(ntplogd_logf, "clock_error");

  if (periodic == false) {
    iteration = 1;
    nonstop   = false;
  }

  if (periodic == true && iteration == 0) {
    // when periodic argument is given but no iteration, then run infinitely
    nonstop = true;
  }

  run_interval_of(interval * NANO_S_PER_S);

  return 0;
}
static void run_interval_of(uint64_t nsec) {

  struct timespec ts_wait, ts_now, ts_sleep;
  bool   terminate = false;
  bool   doSnooze;
  bool   skippedFirst = false; // skip first log

  // Set periodic increment
  uint64_t periodic_inc_ns = nsec;
  struct timespec periodic_inc;
  periodic_inc.tv_sec  = periodic_inc_ns / NANO_S_PER_S;
  periodic_inc.tv_nsec = periodic_inc_ns % NANO_S_PER_S;

  // Set the first completion timestamp
  clock_gettime_tsc(CLOCK_REALTIME, &ts_wait);
  ts_wait.tv_sec++;
  ts_wait.tv_nsec = 0;

  struct clock_error err;
  while (!terminate) {
    // Execute a designated task and print:

    clock_gettime_tsc(CLOCK_REALTIME, &ts_now); 
    //
    // TODO: No real measurement yet
    std::vector<std::string> str_vec;
    std::stringstream ss(exec("chronyc -c tracking"));
    const char delim = ',';
    std::string token;
    while (std::getline(ss, token, delim)) {
      str_vec.push_back(token);
    }

    err.system_time_offset = std::stod(str_vec.at(4));
    err.root_delay         = std::stod(str_vec.at(10));
    err.root_dispersion    = std::stod(str_vec.at(11));

    if (skippedFirst == true) {
      print_clock_err(ntplogd_logf, &ts_now, &err);
      iteration--;
    } else {
      skippedFirst = true;
    }

    // Light sleep until ts_wait or thread runs out of channels to sample:
    doSnooze = true;
    while (doSnooze && !terminate) {

      if (shallTerminate()) {
          // Check terminate condition of the program
          terminate = true;
      } else {
        clock_gettime_tsc(CLOCK_REALTIME, &ts_now); 
        if (compareTS(&ts_now, &ts_wait) > 0) {
          // No more sleep; update completion timestamp and quit sleeping loop:
          addTS(&ts_wait, &ts_wait, &periodic_inc);
          doSnooze = false; 
        } else {
          // Error-retrieval is not due yet, continue to sleep:
          subtractTS(&ts_sleep, &ts_now, &ts_wait);
          uint64_t sleep_ns = ts_sleep.tv_sec * NANO_S_PER_S + ts_sleep.tv_nsec;
          if (sleep_ns > periodic_inc_ns) {
            sleep_ns = periodic_inc_ns;
          }
          std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_ns));
        }
      }
    } // while (sleeping)
  }
}

static bool shallTerminate() {
  if (nonstop || (!nonstop && iteration > 0)) {
    // do not evaluate when nonstop
    return false;
  } 
  return true;
}

static void set_log_file_as(std::ofstream & logf, const char * name) {

  char tmp_buf[255];
  if (0 != getIPDottedQuad(tmp_buf, 255)) {
    std::string errmsg = "getting IP address";
    logFail(__FILE__, __LINE__, errmsg);
    exit(EXIT_FAILURE);
  }

  pid_t pid = getpid();
  int   tid = syscall(SYS_gettid);
  std::ostringstream name_buf;
  name_buf << path_to_output << "/ntplogd" << ":" << tmp_buf << ":"; // IP
  name_buf << pid << ":" << tid << ":" << name << ".txt";
  logf.open(name_buf.str(), std::ios::out | std::ios::trunc);
  if (!logf) {
    std::string errmsg = "open file " + name_buf.str();
    logFail(__FILE__, __LINE__, errmsg);
    exit(EXIT_FAILURE);
  }
}

static void print_clock_err(std::ostream & outf, struct timespec * ts,
                            struct clock_error * err) {
  char buf[1024];

  // epoch_time:ns, system_time_offset, root_del, root_disp, [tv_sec, tv_nsec]
  formatTS(ts, buf, 1024);
  outf << buf << ", " 
  << std::fixed << std::setw(1) << std::setprecision(9) << std::setfill('0')
  << err->system_time_offset << ", "
//<< std::fixed << std::setw(1) << std::setprecision(9) << std::setfill('0')
  << err->root_delay << ", " 
//<< std::fixed << std::setw(1) << std::setprecision(9) << std::setfill('0')
  << err->root_dispersion;
  outf << ", [" << ts->tv_sec << ", " << ts->tv_nsec << ", " << ts->tsc << "]";
  outf << std::endl;
}

