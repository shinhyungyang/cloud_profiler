#ifndef __DOWNSAMPLE_HANDLER_H__
#define __DOWNSAMPLE_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

#define SAMPLING_FACTOR (10)

// The downsample handler keeps every nth sample. The remaining samples
// are discarded.

void DownsampleHandler(void * cl, int64_t tuple);

closure * initDownsampleHandler(log_format format);
// Set up and return the closure of a downsample_handler.

int setArgDownsampleHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by argument 0, which is "n", the
// downsampling factor. The downsampler keeps every nth sample.
// The default-value for n is 10.

#endif
