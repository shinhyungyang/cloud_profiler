#ifndef __MEDIANHANDLER_H__
#define __MEDIANHANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

#define MAX_TS (1000)

// The median handler buffers time-stamps in a buffer of size MAX_TS.
// That is, MAX_TS time-stamps fit into the internal buffer.
// Once the buffer is full, the median over all time-stamps is computed
// and logged to the log-file. The buffer is then emptied.

void MedianHandler(void * cl, int64_t tuple);
// Write median of logged time-stamps if "tuple" is the last tuple fitting
// into the buffer. Otherwise just buffer the tuple.

closure * initMedianHandler(log_format format);
// Set up and return the closure of a median_handler.

int setArgMedianHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by argument 0, which is the alternative
// number of time-stamps to be stored in the buffer. If called, the internal
// buffer is resized from MAX_TS to the provided argument value.
// The provided argument value can be smaller or larger than MAX_TS.
// Re-sizing implicitly empties the buffer.
// On error, the channel is closed and an error-message logged. 

#endif
