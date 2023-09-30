#ifndef __FOX_CONFIG_SERVER_H__
#define __FOX_CONFIG_SERVER_H__

#include "../net_conf.h"

// error codes for config server-client communication

const int8_t ERR_FOX_MULTIPE_START_NODE = -2;
const int8_t ERR_FOX_INVALID_ARG = -3;
const int8_t ERR_FOX_START_NOT_SET = -4;

/**
 * makeFoxStartReply
 * 
 * It makes reply for fox start handler and 
 * stores ip address and port number of start node
 * 
 * @return(bool) : return procesing result, true if successful
 */
bool makeFoxStartReply(conf_req_msg* request, conf_reply_msg* reply, 
  std::vector<conf_entry>::iterator it, uint32_t* line_nr, uint32_t line);


/**
 * makeFoxEndReply
 * 
 * It makes reply for fox end handler containing
 * ip address and port number of the start node
 * 
 * @return(bool) : return procesing result, true if successful
 */
bool makeFoxEndReply(conf_reply_msg* reply, std::vector<conf_entry>::iterator it, 
	uint32_t* line_nr, uint32_t line);

std::string getErrorMsgFox(int8_t errorCode);

#endif // __FOX_CONFIG_SERVER_H__

