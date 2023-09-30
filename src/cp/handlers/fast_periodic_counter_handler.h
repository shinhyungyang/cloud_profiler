#ifndef __FAST_PERIODIC_COUNTER_HANDLER_H__
#define __FAST_PERIODIC_COUNTER_HANDLER_H__ // include-guard

#include "cloud_profiler.h"
#include "../closure.h"

#include <cstdint>

//#define FPC_DEFAULT_PERIOD (5000 * MICRO_S_PER_S)
// Changed #3: sample a counter once per 5000ms (units: ns).

//#define FPC_DEFAULT_PERIOD (150 * MICRO_S_PER_S)
// Changed #2: sample a counter once per 150ms (units: ns).

#define FPC_DEFAULT_PERIOD (10 * MICRO_S_PER_S)
// Changed #1: sample a counter once per 10ms (units: ns).

//#define FPC_DEFAULT_PERIOD (1 * NANO_S_PER_S)
// Default: sample a counter once per second (units: ns).

#define DEFAULT_SNOOZE_PERIOD (MICRO_S_PER_S)
//#define DEFAULT_SNOOZE_PERIOD (NANO_S_PER_S / 10)
// Default: 10Hz frequency for a sampling thread to check its termination
// condition (units: ns).

#define DEFAULT_WAIT_FOR_CH_CLOSE (NANO_S_PER_S / 2)
// Default: wait for 500ms until

// The FastPeriodicCounterHandler counts events and writes the counts
// periodically to the channel's output file. The period is specified in
// nano-seconds in an uint64_t variable. Thus the (theoretical) bounds for the
// handler's period x are 1 ns <= x <= 584 years.
//
// Unlike the PeriodicCounterHandler, sampling of the counter is done by a
// sampling thread, which reduces the event handler function to two
// instructions (incrementing the counter and ret).

void FastPeriodicCounterHandler(void * cl, int64_t tuple);

closure * initFastPeriodicCounterHandler(log_format format);
// Set up and return the closure of a fast periodic counter handler.

int setArgFastPeriodicCounterHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by:
// Argument 0: period of the counter in nano seconds

#endif
