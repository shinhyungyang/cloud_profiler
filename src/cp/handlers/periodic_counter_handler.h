#ifndef __PERIODIC_COUNTER_HANDLER_H__
#define __PERIODIC_COUNTER_HANDLER_H__ // include-guard to prevent double-inclusion

#include "cloud_profiler.h"
#include "../closure.h"

#include <cstdint>

#define DEFAULT_PERIOD (1 * NANO_S_PER_S)
// Default: flush counter once per second

// The PeriodicCounterHandler counts events and writes the counts periodically
// to the channel's output file. The period is specified in nano-seconds in an
// uint64_t variable. Thus the (theoretical) bounds for the handler's period x are
// 1 ns <= x <= 584 years.

void PeriodicCounterHandler(void * cl, int64_t tuple);

closure * initPeriodicCounterHandler(log_format format);
// Set up and return the closure of a periodic counter handler.

int setArgPeriodicCounterHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by:
// Argument 0: period of the counter in nano seconds

#endif
