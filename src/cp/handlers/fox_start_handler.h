#ifndef __FOX_START_HANDLER_H__
#define __FOX_START_HANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>

void FoxStartHandler(void * cl, int64_t tuple);
// Write time-stamp for "tuple" straight to log-file.

closure * initFoxStartHandler(std::string ch_msg, int32_t mychannel, handler_type *ch_h, int64_t *par, log_format * f);
// Set up and return the closure of an fox_start_handler.

std::vector<int> split_ip(const std::string& str, char delim);

#endif
