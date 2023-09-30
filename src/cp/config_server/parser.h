#ifndef __PARSER_H__
#define __PARSER_H__

#include "cloud_profiler.h"
#include "net_conf.h"

#include <cstdint>
#include <string>

void setConfigTable(std::string cnf);
// Select configuration table 'cnf'.

void setDefaultConfigTable();
// Select the default configuration table, i.e., the first table in
// the table from file default_config.h.

bool lookupConfig(conf_req_msg * request, conf_reply_msg * reply,
		  uint32_t * line_nr);
// Look up configuration for channel in 'request' in the configuration table
// and provide the configuration in 'reply'.
// Returns false if configuration cannot be obtained. In this case reply->err
// is set to a negative integer value to tell the client.
// In case of a match, 'line_nr' is the matching entry in the configuration
// table.

bool match(std::string str, std::string expr);
// Returns true if str is a string in the language of expr.

#endif
