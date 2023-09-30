#ifndef __FIRSTLAST_HANDLER_H__
#define __FIRSTLAST_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

// The firstlast handler keeps the first and the last sample. The remaining samples
// are discarded.

void FirstlastHandler(void * cl, int64_t tuple);

closure * initFirstlastHandler(log_format format);
// Set up and return the closure of a FirstlastHandler.

#endif
