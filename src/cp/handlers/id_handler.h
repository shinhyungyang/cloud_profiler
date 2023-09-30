#ifndef __IDHANDLER_H__
#define __IDHANDLER_H__ // include-guard to prevent double-inclusion

#include "time_tsc.h"
#include "../closure.h"
#include <fstream>

#include <cstdint>

void IdentityHandler(void * cl, int64_t tuple);
// Write time-stamp for "tuple" straight to log-file.

closure * initIdentityHandler(log_format format);
// Set up and return the closure of an id_handler.

void writeInAscii(std::ofstream * ch_logf, int64_t tuple, timespec * ts);
// Write log in ASCII form.

void writeInAsciiTsc(std::ofstream * ch_logf, int64_t tuple, timespec * ts);
// Write tuple id and tsc value in ASCII form.

void writeInBinary(std::ofstream * ch_logf, int64_t tuple, timespec * ts);
// Write log in binary form.

void writeInBinaryTsc(std::ofstream * ch_logf, int64_t tuple, timespec * ts);
// Write tuple id and tsc value in binary form.

//void IdentityHandlerRead();
// Do not need this.

void readInBinary(std::ifstream * log_to_read, int64_t * tuple, timespec * ts);
// Read log written in binary form.

#endif
