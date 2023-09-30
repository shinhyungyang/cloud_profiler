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
#include <fstream>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#include <cmath>

#include <sys/select.h> // for using select
#include <x86intrin.h>

////////////////////////////////////////////////////////////////////////////////
// BEGINNING OF __GLOBAL_H__

#define MAX_ALLOC_NR_THREADS 4096

// END OF __BARRIER_H__
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGINNING OF __BARRIER_H__

#ifndef __BARRIER_H__
#define __BARRIER_H__ // prevent against double-inclusion

// Shared-memory barrier implementation until std::experimental::barrier
// is supported by compilers.

// This implementation counts the number of barrier_wait() calls of each thread
// in a separate 64-bit counter. Those counters will overflow after 146 years
// if the thread-ensemble synchronizes at the barrier at a frequency of 4 GHz
// (4*10^9 barrier-synchronizations per second).

extern "C"{
  void barrier_init(int nr_threads);
  // Initialize barrier to specified number of nr_threads

  void barrier_wait(int tid);
  // Busy-wait until all participating threads arrived at the barrier.
}

#endif

// END OF __BARRIER_H__
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGINNING OF __BARRIER_CPP__
#include<cstdint>

struct alignas(CACHELINE_SIZE) aligned_ctr {
  std::atomic<uint64_t> ctr;
};

aligned_ctr ctr[MAX_ALLOC_NR_THREADS];
static int nr_threads;

extern "C"{
  void barrier_init(int nr_threads) {
    ::nr_threads = nr_threads;
    for (int i = 0; i<nr_threads; i++) {
      ctr[i].ctr = 0;
    }
  }
  void barrier_wait(int tid) {
    ctr[tid].ctr += 1;
    uint64_t my_level = ctr[tid].ctr;
    for (int thr = 0; thr < nr_threads; thr++) {
      while (ctr[thr].ctr < my_level) { ; }
    }
  }
}

// END OF __BARRIER_CPP__
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGINNING OF __TIMER_H__

#ifndef __TIMER_H__
#define __TIMER_H__

extern "C" {
void useful_work_us(unsigned long usecs, unsigned long lpjiffy);

void useful_work_ns(unsigned long nsecs, unsigned long lpjiffy);

void __const_udelay(unsigned long xloops, unsigned long lpjiffy) __attribute__((noinline));

unsigned long calibrate_delay(struct tms * dummy_tms);
}

#endif

// END OF __TIMER_H__
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGINNING OF __TIMER_X86_CPP__

#include<sys/times.h>
#include<time.h>
#include<unistd.h>
#define JIFFIES (unsigned long)times(dummy_tms)

const unsigned long LPS_PREC = 8; // defined by default in init/calibrate.c

const unsigned long HZ =sysconf(_SC_CLK_TCK); // NR of clock ticks per second

static inline void delay_loop(unsigned long loops)
{
  asm volatile(
      "     test %0, %0 \n"
      "     jz 3f       \n"
      "     jmp 1f      \n"
      ".align 16        \n"
      "1:   jmp 2f      \n"
      ".align 16        \n"
      "2:   dec %0      \n"
      "     jnz 2b      \n"
      "3:   dec %0      \n"
      :
      :"a"(loops)
      );
}

void useful_work_us(unsigned long usecs, unsigned long lpjiffy) {
  __const_udelay(usecs * 0x000010c7, lpjiffy); /* 2**32 / 1000000(10^6) (rounded up) */
}

void useful_work_ns(unsigned long nsecs, unsigned long lpjiffy) {
  __const_udelay(nsecs * 0x00005, lpjiffy); /* 2**32 / 1000000000(10^9) (rounded up) */
}

void __const_udelay(unsigned long xloops, unsigned long lpjiffy) {
  unsigned long lpj = lpjiffy;
  int d0;
  xloops *= 4;

  asm("mull %%edx"
      :"=d" (xloops), "=&a" (d0)
      :"1" (xloops), "0" (lpj * (HZ / 4)));

  delay_loop(++xloops);
}

// jiffies == times(dummy_tms)
unsigned long calibrate_delay(struct tms * dummy_tms) {
  unsigned long lpj, lpj_base, ticks, loopadd,loopadd_base, chop_limit;
  int trial = 0, band = 0, trial_in_band = 0;
  lpj = (1 << 12);
  ticks = JIFFIES;
  while (ticks == JIFFIES) ;
  ticks = JIFFIES;
  do {
    if (++trial_in_band == (1<<band)) {
      ++band;
      trial_in_band = 0;
    }
    delay_loop(lpj * band); // __delay(lpj * band);
    trial += band;
  } while (ticks == JIFFIES);
  trial -= band;
  loopadd_base = lpj * band;
  lpj_base = lpj * trial;
recalibrate:
  lpj = lpj_base;
  loopadd = loopadd_base;

  chop_limit = lpj >> LPS_PREC;
  while (loopadd > chop_limit) {
    lpj += loopadd;
    ticks = JIFFIES;
    while (ticks == JIFFIES) ;
    ticks = JIFFIES;
    delay_loop(lpj); // __delay(lpj);
    if (JIFFIES != ticks)
      lpj -= loopadd;
    loopadd >>= 1;
  }
  if (lpj + loopadd * 2 == lpj_base + loopadd_base * 2) {
    lpj_base = lpj;
    loopadd_base <<= 2;
    goto recalibrate;
  }
  return lpj;
}

// END OF __TIMER_X86_CPP__
////////////////////////////////////////////////////////////////////////////////

struct alignas(CACHELINE_SIZE) test_vars {
  timespec * ts;
  unsigned long long * logs;
  unsigned int * ids;

  timespec * ts_2;
  unsigned long long * logs_2;
  unsigned int * ids_2;
};
struct test_vars * tvars = nullptr;

void thread_f (int tid, int nr_iterations, unsigned long udelay);

unsigned long long rdtsc_asm();
unsigned long long rdtscp_asm(unsigned int * aux);
unsigned long long rdtsc_gcc();
unsigned long long rdtscp_gcc(unsigned int * aux);

int main(int argc, char ** argv) {
  unsigned long udelay = 200;
  int nr_threads = 1;
  int nr_tests = 1;
  cxxopts::Options options("cp_valtsc",
                           "Validate TSC");
  options.add_options()
  ("i,iterations", "Number of iterations",
   cxxopts::value<std::string>()->default_value("1MB"))
  ("t,threads", "Number of threads",
   cxxopts::value<int>(nr_threads))
  ("d,delay", "delay between tsc calls (usec)",
   cxxopts::value<unsigned long>(udelay))
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
  int array_length = nr_iterations * nr_tests;

  uint64_t size_in_bytes = nr_threads * sizeof(struct test_vars);
  tvars = (struct test_vars *) aligned_alloc (CACHELINE_SIZE, size_in_bytes);
  if (nullptr == tvars) {
    std::cerr << "Error: alloc. of test_vars failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  for (int i=0; i<nr_threads; ++i) {
    tvars[i].logs = nullptr;
    tvars[i].ids = nullptr;

    tvars[i].logs_2 = nullptr;
    tvars[i].ids_2 = nullptr;
  }

  for (int i=0; i<nr_threads; ++i) {
    uint64_t sz = array_length * sizeof(unsigned long long);
    tvars[i].logs = (unsigned long long *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].logs) {
      std::cerr << "Error: alloc. of tvars[" << i << "].logs failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }
    tvars[i].logs_2 = (unsigned long long *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].logs_2) {
      std::cerr << "Error: alloc. of tvars[" << i << "].logs_2 failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }

    sz = array_length * sizeof(unsigned int);
    tvars[i].ids = (unsigned int *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].ids) {
      std::cerr << "Error: alloc. of tvars[" << i << "].ids failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }
    tvars[i].ids_2 = (unsigned int *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].ids_2) {
      std::cerr << "Error: alloc. of tvars[" << i << "].ids_2 failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }

    sz = array_length * sizeof(timespec);
    tvars[i].ts = (timespec *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].ts) {
      std::cerr << "Error: alloc. of tvars[" << i << "].ts failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }
    tvars[i].ts_2 = (timespec *) aligned_alloc (CACHELINE_SIZE, sz);
    if (nullptr == tvars[i].ts_2) {
      std::cerr << "Error: alloc. of tvars[" << i << "].ts_2 failed"
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  for (int i=0; i<nr_threads; ++i) {
    for (int j=0; j<array_length; ++j) {
      tvars[i].logs[j] = 0ULL;
      tvars[i].ids[j] = 0U;
      tvars[i].logs_2[j] = 0ULL;
      tvars[i].ids_2[j] = 0U;

      tvars[i].ts[j].tv_sec = 0;
      tvars[i].ts[j].tv_nsec = 0;
      tvars[i].ts[j].tsc = 0;
      tvars[i].ts[j].aux = 0;

      tvars[i].ts_2[j].tv_sec = 0;
      tvars[i].ts_2[j].tv_nsec = 0;
      tvars[i].ts_2[j].tsc = 0;
      tvars[i].ts_2[j].aux = 0;
    }
  }

  char buf[1024];
  timespec now;
  getTS(&now);
  formatTS(&now, buf, sizeof(buf));


  barrier_init(nr_threads);

  // Multi-threaded tests:
  std::thread tids[nr_threads];
  for (int i = 0; i<nr_threads; i++) {
    tids[i] = std::thread(thread_f, i, nr_iterations, udelay);
  }
  for (int i = 0; i<nr_threads; i++) {
    tids[i].join();
  }

  //
  // Logging
  //
  std::fstream logf;
  std::ostringstream logf_name;
  logf_name << "/tmp/calctsc_log_" << buf << ".txt";
  logf.open(logf_name.str(), std::fstream::out);

  std::ostringstream header;
  header << "start [tv_sec, tv_nsec]" << ", ";
  header << "[tsc, tsc_aux]" << ", ";
  header << "end [tv_sec, tv_nsec]" << ", ";
  header << "[tsc, tsc_aux]" << ", ";
  header << "TSC Freq" << std::endl;
  logf << header.str();

  for (int i = 0; i<array_length; ++i) {
    for (int j = 0; j<nr_threads; ++j) {
      std::ostringstream line;
      unsigned long long dur_sec = 0ULL;
      unsigned long long dur_tsc = 0ULL;
      unsigned tsc_freq = 0ULL;

      line << tvars[j].ts[i].tv_sec << ", ";
      line << tvars[j].ts[i].tv_nsec << ", ";
      line << tvars[j].logs[i] << ", ";
      line << tvars[j].ids[i] << ", ";

      line << tvars[j].ts_2[i].tv_sec << ", ";
      line << tvars[j].ts_2[i].tv_nsec << ", ";
      line << tvars[j].logs_2[i] << ", ";
      line << tvars[j].ids_2[i] << ", ";

      dur_sec = ((tvars[j].ts_2[i].tv_sec * NANO_S_PER_S) + tvars[j].ts_2[i].tv_nsec) -
                ((tvars[j].ts[i].tv_sec * NANO_S_PER_S) + tvars[j].ts[i].tv_nsec);
      dur_tsc = tvars[j].logs_2[i] - tvars[j].logs[i];
      tsc_freq = (unsigned) ((double) dur_tsc / ((double) dur_sec / NANO_S_PER_S));
      line << tsc_freq << std::endl;

      logf << line.str();
    }
  }
  logf.close();

  for (int i=0; i<nr_threads; ++i) {
    if (nullptr != tvars[i].logs) {
      free(tvars[i].logs);
    }
    if (nullptr != tvars[i].ids) {
      free(tvars[i].ids);
    }
    if (nullptr != tvars[i].logs_2) {
      free(tvars[i].logs_2);
    }
    if (nullptr != tvars[i].ids_2) {
      free(tvars[i].ids_2);
    }
    if (nullptr != tvars[i].ts) {
      free(tvars[i].ts);
    }
    if (nullptr != tvars[i].ts_2) {
      free(tvars[i].ts_2);
    }
  }

  if (nullptr != tvars) {
    free(tvars);
  }

  return 0;
}

void spend (timespec * cur_ts, const timespec * end_ts) {
  while (compareTS(cur_ts, end_ts) < 0) {
    getTS(cur_ts);
  }
}

void thread_f (int tid, int nr_iterations, unsigned long udelay) {

  const timespec TS_SLEEP = { .tv_sec = (60 * 5), .tv_nsec = 0 }; // 1 ms
  struct tms * dummy_tms = nullptr;
  unsigned long loops_per_jiffy = 0UL;

  loops_per_jiffy = calibrate_delay(dummy_tms);

  barrier_wait(tid);

  timespec _ts;
  for (int i=0; i<nr_iterations; ++i) {
    clock_gettime_tsc(CLOCK_REALTIME, &_ts);
    tvars[tid].logs[i] = _ts.tsc;
    tvars[tid].ids[i] = _ts.aux;
    tvars[tid].ts[i].tv_sec = _ts.tv_sec;
    tvars[tid].ts[i].tv_nsec = _ts.tv_nsec;

    pselect(0, NULL, NULL, NULL, &TS_SLEEP, NULL);

    clock_gettime_tsc(CLOCK_REALTIME, &_ts);
    tvars[tid].logs_2[i] = _ts.tsc;
    tvars[tid].ids_2[i] = _ts.aux;
    tvars[tid].ts_2[i].tv_sec = _ts.tv_sec;
    tvars[tid].ts_2[i].tv_nsec = _ts.tv_nsec;
  }
}


unsigned long long rdtsc_asm(void) {
#if defined (__x86_64__)
  unsigned low, high;
  asm volatile("rdtsc" : "=a" (low), "=d" (high));
  return low | ((unsigned long long)high) << 32;
#elif defined (__aarch64__)
  uint64_t val;
  asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
  return val;
#else
  #error "Unsupported architecture"
#endif
}

unsigned long long rdtscp_asm(unsigned int * aux) {
#if defined (__x86_64__)
  uint64_t rax,rdx;
  asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (*aux) : : );
  return (rdx << 32) + rax;
#elif defined (__aarch64__)
  ;
#else
  #error "Unsupported architecture"
#endif
}

unsigned long long rdtsc_gcc(void) {
  return __rdtsc();
}

unsigned long long rdtscp_gcc(unsigned int * aux) {
  return __rdtscp(aux);
}
