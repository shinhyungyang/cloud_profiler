#ifndef __GPIORTT_HANDLER_H__
#define __GPIORTT_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

#define PMU_MODULE "pmu_access"
#define CPU_PERF   "performance"
#define MAX_FREQ   "max_freq"
#define MIN_FREQ   "min_freq"

// The GPIO RTT handler logs TODO
// ...
// ...
//
// A tuple of tuple id <tuple_id> is logged iff:
//   ...
// Otherwise a tuple is not logged.

void GPIORTTHandler_client(void * cl, int64_t tuple);

closure * initGPIORTTHandler(log_format format);
// Set up and return the closure of an GPIO RTT handler.

int setArgGPIORTTHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by:
// TODO

#endif
