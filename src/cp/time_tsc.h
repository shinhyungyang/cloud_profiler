#ifndef _TIME_TSC_H_
#define _TIME_TSC_H_

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC

#ifndef __timespec_defined
#define __timespec_defined 1

//#include <bits/types/clockid_t.h>
#include <bits/types.h>

/*
 * FIXME:
 * rename this struct
 * in cp_cpptest.cpp, struct timespec refers to that from C library
 * but, in getTS function it assumes that the input pointer is type of struct timespec
 * from this header
 * and their definitions differs.
 * in clock_gettime_tsc function, writes on the third field which does not appear
 * in the definition from C library, so finally it results in Segmentation faults.
 */

struct timespec {
  __time_t              tv_sec;     /* seconds */
  __syscall_slong_t     tv_nsec;    /* nanoseconds */
  unsigned long long    tsc;        /* rdtsc */
  unsigned int          aux;        /* CPU and node number */
};

#endif /* __timespec_defined */
#endif /* _STRUCT_TIMESPEC */

/*
 * cloud_profiler defined clocks
 *
 * 0~11: reserved by Linux kernel
 * 16: MAX_CLOCKS
 */
#define CLOCK_REALTIME_CP 12

// Extended version of clock_gettime
int clock_gettime_tsc(__clockid_t clk_id, struct timespec *tp);
unsigned long long rdtsc(void);
unsigned long long rdtscp(unsigned int * aux);
void clock_gettime_tsc_only(__clockid_t clk_id, struct timespec *tp);

#endif /* _TIME_TSC_H_ */
