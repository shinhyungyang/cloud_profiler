#include "time_tsc.h"

extern "C" {
#include "net.h"
}

#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <vector>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
}

#include "cloud_profiler.h"
#include "globals.h"
#include "log.h"
#include "counters_aarch64.h"
#include "barrier.h"

#include "cxxopts.hpp"

#define MAX_NR_THREADS 6

struct test_info {
  struct timespec ts;

  double    offset;       // offset of current freq from the initial frequency
  double    offset_avg;   // offset of current freq from the averaged frequency

  uint64_t  cycles_ref;   // Cycles of the first iteration
  double    nsec_ref;     // UTC nanoseconds of the first iteration
  double    freq_ref;     // Counter freqnency of the first iteration

  uint64_t  cycles_now;   // Cycles from current iteration
  double    nsec_now;     // UTC nanoseconds from current iteration
  double    freq_now;     // Counter frequency from current iteration

  uint64_t  cycles_tot;   // Cycles from accumulation
  double    nsec_tot;     // UTC nanoseconds from accumulation
  double    freq_avg;     // Counter freqnency from accumulation
};

static void set_log_file_as(std::ofstream & logf, const char * name);
static void print_test_info(std::ostream  & outf, struct timespec * ts,
                            struct test_info * ti);

void thread_f(long tid, long iter, bool infinite) {
  struct timespec ts_start, ts_end;
  uint64_t start, end;
  uint64_t cycles;

  std::vector<struct test_info> measure_list;

  std::ofstream logf;
  set_log_file_as(logf, "armfreq");

  std::mt19937_64 rng;
  uint64_t timeSeed =
    std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
  rng.seed(ss);
  std::uniform_real_distribution<double> unif(0.10, 0.11);
  uint64_t decrement =                     // Get integer 1 by random to avoid
    static_cast<uint64_t>(unif(rng) * 10); // loop optimization

  if (infinite) {
    while (decrement == 1) ; // decrement is always 1; so run infinitely
  } else {
    /*********
     * Warm-up
     *********/
    uint64_t random_number = decrement * NANO_S_PER_S;
    do {
      random_number -= decrement;
    } while (random_number > 0);

    /*******************
     * Measure frequency
     *******************/
    struct test_info ti;
    ti.freq_ref   = 0.0;
    ti.cycles_tot = 0;
    ti.nsec_tot   = 0.0;

    for (long i = 0; i < iter; ++i) {
      clock_gettime_tsc(CLOCK_REALTIME, &ts_start);         //////////////////////////
      asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(start));

      random_number = decrement * NANO_S_PER_S * 10;
      do {
        random_number -= decrement;
      } while (random_number > 0);

      clock_gettime_tsc(CLOCK_REALTIME, &ts_end);
      asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(end));  //////////////////////////

      double nsec =   (ts_end.tv_sec-ts_start.tv_sec) * NANO_S_PER_S * 1.0;
      nsec        +=  (double)(ts_end.tv_nsec-ts_start.tv_nsec);
      cycles      =   end-start;
      double freq =   (((double)cycles) / nsec);
      freq        *=  NANO_S_PER_S;

      if (ti.freq_ref == 0.0) {
        ti.cycles_ref = cycles;
        ti.nsec_ref   = nsec;
        ti.freq_ref   = freq;
      }

      ti.cycles_now = cycles;
      ti.nsec_now   = nsec;
      ti.freq_now   = freq;

      ti.cycles_tot +=  cycles;
      ti.nsec_tot   +=  nsec;
      ti.freq_avg   =   (((double)ti.cycles_tot) / ti.nsec_tot) * NANO_S_PER_S;

      ti.offset     = ti.freq_now - ti.freq_ref;
      ti.offset_avg = ti.freq_now - ti.freq_avg;

      ti.ts.tv_sec  = ts_end.tv_sec;
      ti.ts.tv_nsec = ts_end.tv_nsec;
      ti.ts.tsc     = ts_end.tsc;

      measure_list.push_back(ti);
    }

    barrier_wait(tid);

    for (auto t = measure_list.begin(); t != measure_list.end(); ++t) {
      print_test_info(logf, &((*t).ts), &((*t)));
    }
    //asm volatile("mrs %0, cntfrq_el0" : "=r"(/*uint32_t variable here*/));
  }
}

int main(int argc, char ** argv) {

  bool console_log = false;
  long iteration = 0;
  bool infinite = false;
  bool test = false;

  cxxopts::Options options("armfreq",
                           "cloud_profiler ARMv8 counter measurement");
  options.add_options()
  ("c,console",     "Write log-file output to console",
   cxxopts::value<bool>(console_log))
  ("i,iteration",   "The number of measurements to do",
   cxxopts::value<long>(iteration))
  ("f,infinite", "Run infinitely regardless of iteration")
  ("t,test", "Run an instruction test")
  ("h,help", "Usage information")
  ("positional",
    "Positional arguments: these are the arguments that are entered "
    "without an option", cxxopts::value<std::vector<std::string>>())
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  if (options.count("f")) {
    infinite = true;
  }

  if (options.count("t")) {
    test = true;
    uint64_t u64 = 0UL;
    std::cout << "sizeof:" << std::endl;
    std::cout << "    uint32_t: " << sizeof(uint32_t) << std::endl;
    std::cout << "    unsigned short: " << sizeof(unsigned short) << std::endl;
    std::cout << "    unsigned: " << sizeof(unsigned) << std::endl;
    std::cout << "    unsigned int: " << sizeof(unsigned int) << std::endl;
    std::cout << "    unsigned long: " << sizeof(unsigned long) << std::endl;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(u64));
    std::cout << "The frequency of the system counter: " << std::endl;
    std::cout << "    " << u64 << std::endl;
    asm volatile("mrs %0, cntpct_el0" : "=r"(u64));
    std::cout << "Current System counter value: " << std::endl;
    std::cout << "    " << u64 << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set Console log
  setLogFileName("/tmp/armfreq_logfile");
  toggleConsoleLog(console_log);
  std::cout << "Console log: ";
  if (console_log) {
    std::cout << "enabled" << std::endl;
  } else {
    std::cout << "disabled" << std::endl;
  }

  barrier_init(MAX_NR_THREADS);

  std::thread thrs[MAX_NR_THREADS];

  for (long i = 0; i < MAX_NR_THREADS; i++) {
    thrs[i] = std::thread(thread_f, i, iteration, infinite);
  }

  for (int i = 0; i < MAX_NR_THREADS; i++) {
    thrs[i].join();
  }
  return 0;
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
  name_buf << path_to_output << "/cloud_profiler" << ":" << tmp_buf << ":"; // IP
  name_buf << pid << ":" << tid << ":" << name << ".txt";
  logf.open(name_buf.str(), std::ios::out | std::ios::trunc);
  if (!logf) {
    std::string errmsg = "open file " + name_buf.str();
    logFail(__FILE__, __LINE__, errmsg);
    exit(EXIT_FAILURE);
  }
  logf << "# epoch_time:ns, cur_freq, avg_freq, offset, offset_avg, "
       << "[tv_sec, tv_nsec, tsc]" << std::endl;
}

static void print_test_info(std::ostream & outf, struct timespec * ts,
                            struct test_info * ti) {
  char buf[1024];

  // epoch_time:ns, cur_freq, avg_freq, offset, offset_avg, [tv_sec, tv_nsec, tsc]
  formatTS(ts, buf, 1024);
  outf << buf 
       << ", " << std::fixed << std::setw(12) << std::setprecision(0) << ti->freq_now
       << ", " << std::fixed << std::setw(12) << std::setprecision(0) << ti->freq_avg
       << ", " << std::setw(9)  << ti->offset
       << ", " << std::setw(9)  << ti->offset_avg
       << ", [" << ts->tv_sec << ", " << ts->tv_nsec << ", " << ts->tsc << "]"
       << std::endl;
}

