#ifndef __NET_CONF_H__
#define __NET_CONF_H__

#include "cloud_profiler.h"
#include "globals.h"

#include <cstdint>
#include <string>

#define MAX_STR_SIZE (128)

/* Success code for return value of lookupConfig, should be in reply.err */
const int8_t CONF_LOOKUP_SUCCESS = 0;
const int8_t ERR_CONF_NOT_FOUND = -1;

/**
 * Retrieve the channel configuration for 'ch'.
 * 
 * @Return:
 *   h(handler_type*): The handler_type will be returned in *h. 
 *   format(log_format*): The log_format will be returned in *format.
 *   par(int64_t*): the array of parameters if the handler requires parameters.
 *   return value(int32_t): 
 *      The function returns 0 if a configuration was retrieved 
 *      from the configuration server, and non-zero with erro code otherwise.
 *      error code is defined in net_conf.h
 * 
 * @Caution:
 * It is the responsibility of the caller to make sure that *par 
 * contains sufficient space for all parameters sent from the configuration server.
  */
bool getConfigFromServer(std::string ch_name, int32_t ch_nr, handler_type * h, 
                         int64_t * par, log_format * format);
/**
 * Fully explicated parameter version of the above function.
  */
bool getConfigFromServer(std::string sv_dotted_quad,
               uint32_t    sv_port,
               std::string ch_name,
               std::string ch_dotted_quad,
               uint32_t    ch_pid,
               uint32_t    ch_tid,
               int32_t     ch_nr,
               handler_type * h, int64_t * par, log_format * format);

// Configuration request message sent from client to the configuration server:
struct conf_req_msg {
  char ch_name[MAX_STR_SIZE];
  char ch_dotted_quad[MAX_STR_SIZE];
  uint32_t ch_pid;
  uint32_t ch_tid;
  int32_t  ch_nr;
  int32_t nr_parameters;
  int64_t par_value[conf_server_max_param];
  conf_req_msg(): nr_parameters(0) {}
  std::string toStr(); // Return a string representation of the message.
};

// Configuration reply message sent from configuration server back to
// the client:
struct conf_reply_msg {
  int8_t  err; // 0: ok, non-zero otherwise
  handler_type h_type;
  log_format log_f;
  uint8_t nr_parameters;
  int64_t par_value[conf_server_max_param]; // parameter value (at index)
  conf_reply_msg():nr_parameters(0) {}
  std::string toStr(); // Return a string representation of the message.
};


// Configuration entry from configuration file; follows the structure of the
// client request conf_req_msg, except that we use strings to facilitate
// parsing.
struct conf_entry {
  // Client part:
  std::string ch_name;
  std::string ch_dotted_quad;
  std::string ch_pid;
  std::string ch_tid;
  std::string ch_nr;
  // Configuration part:
  handler_type h_type;
  log_format log_f;
  uint8_t nr_parameters;
  int64_t par_value[conf_server_max_param];
};

#endif
