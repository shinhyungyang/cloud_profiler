#ifndef __FOX_END_HANDLER_H__
#define __FOX_END_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

const int FOX_END_RETRY = 10;
const int FOX_END_SLEEP = 1;

void FoxEndHandler(void * cl, int64_t tuple);
// Write time-stamp for "tuple" straight to log-file.

int setArgFoxEndHandler(void * cl, int32_t nr_par, int64_t par);

closure * initFoxEndHandler(std::string ch_msg, int32_t mychannel, handler_type *ch_h, int64_t *par, log_format * f);
// Set up and return the closure of an fox_end_handler.

#endif
