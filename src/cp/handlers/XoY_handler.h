#ifndef __XOY_HANDLER_H__
#define __XOY_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

#define X_FACTOR (100)
#define Y_FACTOR (16384) // 2**14, must be power of 2

// The XoY downsample handler logs the first X tuple IDs of every batch of Y
// tuple IDs. Y is required to be a power of two, to be able to use
// bit-operations instead of the more costy modulo operation.
//
// A tuple of tuple id <tuple_id> is logged iff:
//   <tuple_id> modulo Y_FACTOR < X_FACTOR
// Otherwise a tuple is not logged.

void XoYHandler(void * cl, int64_t tuple);

closure * initXoYHandler(log_format format);
// Set up and return the closure of an XoY handler.

int setArgXoYHandler(void * cl, int32_t arg_number, int64_t arg_value);
// This handler can be parameterized by:
// Argument 0: the X factor.
// Argument 1: the Y factor.
// The channel is closed in error cases:
// (1)  Y < X
// (2)  Y not a power of 2
// (3)  0 >= X, Y
//
// Thus: the user must increase the Y_FACTOR before the X_FACTOR to prevent
// Condition (1) to hold!

#endif
