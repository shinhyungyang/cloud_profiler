#ifndef __NULLHANDLER_H__
#define __NULLHANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

void NullHandler(void * cl, int64_t tuple);
// Discards log information.

closure * initNullHandler(log_format format);
// Set up and return the closure of a null_handler.

#endif
