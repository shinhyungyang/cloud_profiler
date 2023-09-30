#include <cstdlib>
#include "../net_conf.h"
#include "fox_config_server.h"

static struct FoxStartInfo
{
    int64_t ip[4] = {-1, -1, -1, -1};
    int64_t port = -1;
    bool isSet = false;
} foxStartInfo;

bool makeFoxStartReply(conf_req_msg *request, conf_reply_msg *reply,
                       std::vector<conf_entry>::iterator it, uint32_t *line_nr, uint32_t line)
{
    reply->h_type = it->h_type;
    if (foxStartInfo.isSet)
    {
        
        reply->err = ERR_FOX_MULTIPE_START_NODE;
        return false;
    }
    // storing ip address
    for (int32_t i = 0; i < 4; ++i)
    {
        foxStartInfo.ip[i] = request->par_value[i + 1];
    }
    // storing port number
    foxStartInfo.port = request->par_value[5];
    if (request->nr_parameters == 5)
    {
        foxStartInfo.isSet = true;
    }
    // Set up reply message:
    reply->err = CONF_LOOKUP_SUCCESS;

    // Copy parameter values:
    reply->nr_parameters = it->nr_parameters;
    for (uint32_t par = 0; par < reply->nr_parameters; par++)
    {
        reply->par_value[par] = it->par_value[par];
    }
    *line_nr = line;
    return true;
}

bool makeFoxEndReply(conf_reply_msg *reply,
                     std::vector<conf_entry>::iterator it, uint32_t *line_nr, uint32_t line)
{
    reply->h_type = it->h_type;
    if (!foxStartInfo.isSet)
    {
        reply->nr_parameters = 0;
        reply->err = ERR_FOX_START_NOT_SET;
        return false; // error
    }
    // Set up reply message:
    reply->err = CONF_LOOKUP_SUCCESS;
    reply->nr_parameters = 5u; // ip address(4) + port number(1)
    // Copy parameter values:
    for (int32_t i = 0; i < 4; ++i)
    {
        reply->par_value[i] = foxStartInfo.ip[i];
    }
    reply->par_value[4] = foxStartInfo.port;
    *line_nr = line;
    return true;
}

std::string getErrorMsgFox(int8_t errorCode)
{
    switch (errorCode)
    {
        // when multiple node request fox_start handler, terminate
    case ERR_FOX_MULTIPE_START_NODE:
    {
        return std::string("Multiple node request \'fox_start\' handler");
    }
    // when fox_start handler requested, but not provide ip address and port
    case ERR_FOX_INVALID_ARG:
    {
        return std::string("\'fox_start\' handler need 5 parameters");
    }
    // when fox_start handler requested, but not provide ip address and port
    case ERR_FOX_START_NOT_SET:
    {
        return std::string("\'fox_end\' handler requested, but \'fox_start\' is not set");
    }   
    }
    return std::string("unknown error for fox");
}