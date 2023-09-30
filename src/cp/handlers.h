#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include "cloud_profiler.h"

#include <string>

bool str_to_handler(std::string s, handler_type * t);
// Convert a string to a handler type.
// Returns false if string s does not represent a valid handler type.

bool handler_to_str(handler_type t, std::string ** s);
// Convert a handler type t to string s.
// Returns false if the conversion cannot be achieved.

bool format_to_str(log_format f, std::string * s);
// Convert a log format f to string s.
// Returns false if the conversion cannot be achieved.

bool str_to_format(std::string s, log_format * f);
// Convert a string to a log format.
// Returns false if string s does not represent a valid log format.

#endif
