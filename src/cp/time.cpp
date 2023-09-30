#define __USE_POSIX // for localtime_r
#define _POSIX_C_SOURCE 199309

#include "time_tsc.h"

#include "cloud_profiler.h"
#include "globals.h"
extern "C" {
#include "util.h"
}

#include <atomic>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <cstdint>

inline int64_t getResolutionNano(void) {
  struct timespec clock_res;
  int ret = clock_getres(CLOCK_REALTIME, &clock_res);  
  if (ret == 0)
    return clock_res.tv_nsec;
  return -1;
}

int getTS(struct timespec *ts) {
  return clock_gettime_tsc(CLOCK_REALTIME, ts);
}

int formatTS(struct timespec *ts,
             char *s,
             size_t max,
             const std::string format)
{
  char tmp_buf[max];
  struct tm calendar_time;
  if (NULL==localtime_r(&ts->tv_sec, &calendar_time))
    return  -1;
  if (0==strftime(tmp_buf, max, format.c_str(), &calendar_time))
    return -1;
  if (0 > snprintf(s, max, "%s:%09ld", tmp_buf, ts->tv_nsec))
    return -1;
  return 0;
}

int formatTS(struct timespec *ts, char *s, size_t max) {
  static const std::string def_format = "%Y-%m-%d_%H:%M:%S";
  return formatTS(ts, s, max, def_format);
}

void addTS(struct timespec *result,
           const struct timespec *t1,
           const struct timespec *t2)
{
  result->tv_sec = t1->tv_sec + t2->tv_sec;
  result->tv_nsec = t1->tv_nsec + t2->tv_nsec;
  if (result->tv_nsec >= (int64_t)NANO_S_PER_S) {
    result->tv_sec++;
    result->tv_nsec -= NANO_S_PER_S;
  } 
  result->tsc = t1->tsc + t2->tsc;
}

void subtractTS(struct timespec * result,
                struct timespec * start,
                struct timespec * end)
{
  if ((end->tv_nsec - start->tv_nsec) < 0) {
    result->tv_sec  = end->tv_sec - start->tv_sec - 1;
    result->tv_nsec = end->tv_nsec - start->tv_nsec + NANO_S_PER_S;
  } else {
    result->tv_sec  = end->tv_sec - start->tv_sec;
    result->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
  result->tsc = end->tsc - start->tsc;
}

int divideTS(struct timespec * result, struct timespec * t, int div) {
  if ((div==0) ||
      (t->tv_sec > (int64_t)(18446744073709551614UL / NANO_S_PER_S)))
    return -1;
  uint64_t ns = (t->tv_sec * NANO_S_PER_S + t->tv_nsec) / div;
  result->tv_sec  = ns / NANO_S_PER_S;
  result->tv_nsec = ns % NANO_S_PER_S; 
  result->tsc = t->tsc / div;
  return 0;
}

int compareTS(const struct timespec * t1, const struct timespec * t2) {
  if (t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec) {
    return 0;
  }
  if (t1->tv_sec < t2->tv_sec ||
       (t1->tv_sec == t2->tv_sec && t1->tv_nsec < t2->tv_nsec)) {
    return -1;
  }
  return 1;
}

int formatDuration(uint64_t t_secs, uint64_t ns,
                   char *s, size_t max, bool units) {
  std::string unit_str;
  int ret;
  if (t_secs > S_PER_DAY) {
    unit_str = units?" days_hh:mm:ss:ns":"";
    uint32_t days = t_secs / S_PER_DAY;
    t_secs = t_secs % S_PER_DAY; 
    uint8_t hours = t_secs / S_PER_HOUR;
    t_secs = t_secs % S_PER_HOUR;
    uint8_t mins = t_secs / 60;
    uint8_t secs = t_secs % 60;
    ret = snprintf(s, max, "%d_%02d:%02d:%02d:%09ld%s",
                   days, hours, mins, secs, ns, unit_str.c_str());
  } else if (t_secs > S_PER_HOUR) {
    unit_str = units?" hh:mm:ss:ns":"";
    uint8_t hours = t_secs / S_PER_HOUR;
    t_secs = t_secs % S_PER_HOUR;
    uint8_t mins = t_secs / 60;
    uint8_t secs = t_secs % 60;
    ret = snprintf(s, max, "%02d:%02d:%02d:%09ld%s",
                   hours, mins, secs, ns, unit_str.c_str());
  } else if (t_secs > S_PER_MIN) {
    unit_str = units?" mm:ss:ns":"";
    uint8_t mins = t_secs / 60;
    uint8_t secs = t_secs % 60;
    ret = snprintf(s, max, "%02d:%02d:%09ld%s",
                   mins, secs, ns, unit_str.c_str());
  } else if (t_secs > 0) {
    unit_str = units?" ss:ns":"";
    ret = snprintf(s, max, "%2d:%09ld%s",
                   (uint8_t)t_secs, ns, unit_str.c_str());
  } else {
    unit_str = units?" ns":"";
    char buf[1024];
    l2aCommas(ns, buf);
    ret = snprintf(s, max, "%s%s", buf, unit_str.c_str());
  }
  if (0 > ret) {
    return 1;
  }
  return 0;
}

int formatDuration(struct timespec *ts, char *s, size_t max, bool units) {
  return formatDuration(ts->tv_sec, ts->tv_nsec, s, max, units);
}

int formatDuration(uint64_t ns, char *s, size_t max, bool units) {
  uint64_t split_s  = ns / NANO_S_PER_S;
  uint64_t split_ns = ns % NANO_S_PER_S;
  return formatDuration(split_s, split_ns, s, max, units);
}

void cpuid() {
  // We don't need the result from cpuid
  asm volatile("CPUID"::: "eax","ebx","ecx","edx", "memory");
}


unsigned long long rdtsc(void) {
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

// Using inline assembly with serialization instructions
// https://stackoverflow.com/a/48523760/936369
unsigned long long rdtscp(unsigned int * aux) {
#if defined (__x86_64__)
  uint64_t rax,rdx;
//asm volatile("CPUID"::: "eax","ebx","ecx","edx", "memory");
  asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (*aux) : : );
//asm volatile("CPUID"::: "eax","ebx","ecx","edx", "memory");
  asm volatile("lfence" ::: "memory");
  return (rdx << 32) + rax;
#elif defined (__aarch64__)
  ;
#else
  #error "Unsupported architecture"
#endif
}

int clock_gettime_tsc(clockid_t clk_id, struct timespec *tp) {
  //test
  clockid_t test_id = clk_id;
  //clockid_t test_id = CLOCK_MONOTONIC_RAW;
  int retval = clock_gettime(test_id, tp);
  //int retval = clock_gettime(clk_id, tp);
  unsigned int aux = 0U; // for rdtscp
  tp->tsc = rdtscp(&aux);
  tp->aux = aux;
//tp->tsc = rdtsc();
  return retval;
}

void clock_gettime_tsc_only(clockid_t clk_id, struct timespec *tp) {
  tp->tsc = rdtsc();
}
