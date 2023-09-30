#ifndef __TSC_H__
#define __TSC_H__ // prevent against double-inclusion


bool check_rdtsc(void);
// Returns true if CPU we're running on supports the rtdsc instruction.
// The check is conducted via the cpuid instruction.


bool check_rdtscp(void);
// Returns true if CPU we're running on supports the rtdscp instruction.
// The check is conducted via the cpuid instruction.

unsigned long get_tsc_frequency(void);
// Returns CPU's timestamp-counter freqency in hz

#endif
