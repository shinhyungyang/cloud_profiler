#include "net_conf.h"
#include "globals.h"
#include "handlers.h"
#include "log.h"
#include "fox/fox_config_server.h"

extern "C" {
#include "net.h"
}

#include <zmq.hpp>
#include <string>
#include <iostream>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
}

bool getConfigFromServer(std::string ch_name, int32_t ch_nr, handler_type * h,
               int64_t * par, log_format * format)
{
  std::string ch_dotted_quad;
  char tmp_buf[MAX_STR_SIZE];
  uint32_t ch_pid, ch_tid;
  // Assemble parameters from point of view of a cloud_profiler
  // run-time instance. In particular, there are no command-line
  // arguments, so we need to resort to default values:
  if (0 != getIPDottedQuad(tmp_buf, MAX_STR_SIZE))
  {
    strncpy(tmp_buf, "NO.IP.ADDR", MAX_STR_SIZE);
    logFail(__FILE__, __LINE__, "unable to obtain channel's IP address");
  }
  ch_dotted_quad = tmp_buf;
  ch_pid = getpid();
  ch_tid = syscall(SYS_gettid);
  // Retrieve configuration:
  return getConfigFromServer(conf_server_default_ip,
                             conf_server_default_port,
                             ch_name, ch_dotted_quad, ch_pid, ch_tid, ch_nr,
                             h, par, format);
}

bool getConfigFromServer(std::string sv_dotted_quad,
                         uint32_t sv_port,
                         std::string ch_name,
                         std::string ch_dotted_quad,
                         uint32_t ch_pid,
                         uint32_t ch_tid,
                         int32_t ch_nr,
                         handler_type *h, int64_t *par, log_format *format)
{
  // Prepare request message:
  
  conf_req_msg req_msg;
  strncpy(req_msg.ch_name, ch_name.c_str(), MAX_STR_SIZE);
  strncpy(req_msg.ch_dotted_quad, ch_dotted_quad.c_str(), MAX_STR_SIZE);
  req_msg.ch_pid = ch_pid;
  req_msg.ch_tid = ch_tid;
  req_msg.ch_nr = ch_nr;
  
  req_msg.nr_parameters = par[0];
  
  if (par[0] > 0) {
    std::memcpy(req_msg.par_value+1, par+1, par[0] * sizeof(par[0]));
  }
  // Log request:
  logStatus(__FILE__, __LINE__, req_msg.toStr());

  // zmq:
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REQ);
  // Open socket (IP, port):
  std::ostringstream ep;
  ep << "tcp://" << sv_dotted_quad << ":";
  ep << sv_port;
  socket.connect (ep.str());
  // Send request:
  zmq::message_t request((void *) &req_msg, sizeof(req_msg)); 
  socket.send (request);

  // Receive the reply:
  zmq::message_t reply;
  socket.recv (&reply);
  conf_reply_msg * msg = (conf_reply_msg *) reply.data();
  // FIXME: messages must be deleted!
  socket.disconnect(ep.str());
  socket.close();

  // Log the reply:
  logStatus(__FILE__, __LINE__, msg->toStr());

  // Copy out data from the reply message:
  std::string * s;
  static const std::string GET_CONFIG_FAIL_ERR_MSG("ERROR: getConfigFromServer failed: ");
  if (!handler_to_str(msg->h_type, &s) || msg->err) {
    switch(msg->err){
      // Configuration cannot be obtained. 'reply_msg' already contains the
      // error value so the client will find out about it.
      // Here we only enter the incident in the log.
      case ERR_CONF_NOT_FOUND:
        logStatus(__FILE__, __LINE__, 
          std::string(GET_CONFIG_FAIL_ERR_MSG+"Configuration cannot be obtained (no matching entry for request)"));
        break;
      // when multiple node request fox_start handler, terminate
      case ERR_FOX_MULTIPE_START_NODE:
        logStatus(__FILE__, __LINE__, 
          std::string(GET_CONFIG_FAIL_ERR_MSG+"Multiple node request \'fox_start\' handler"));
        break;
      // when fox_start handler requested, but not provide ip address and port
      case ERR_FOX_INVALID_ARG:
        logStatus(__FILE__, __LINE__, 
          std::string(GET_CONFIG_FAIL_ERR_MSG+"\'fox_start\' handler need 5 parameters"));
        break;
      // when fox_start handler requested, but not provide ip address and port
      case ERR_FOX_START_NOT_SET:
        logStatus(__FILE__, __LINE__, 
          std::string(GET_CONFIG_FAIL_ERR_MSG+"\'fox_end\' handler requested, but \'fox_start\' is not set"));
        *h = msg->h_type;
      //break;
        return true; // Returning false results in creating a DefectHandler.
                     // Return true instead, to request config_server.
    }
    return false;
  } else {
    *h = msg->h_type;
    *format = msg->log_f;
    par[0] = msg->nr_parameters;
    if (msg->nr_parameters > 0) {
      // Copy out parameters:
      for (int i=0; i<msg->nr_parameters; i++) {
        par[i+1] = msg->par_value[i];
      }
    }
  }
  return true;
}

std::string conf_req_msg::toStr() {
  std::ostringstream answer;
  answer << "Request: ch <" << ch_name << ">, ";
  answer << "IP <" << ch_dotted_quad << ">, ";
  answer << "PID <" << ch_pid << ">, ";
  answer << "TID <" << ch_tid << ">, ";
  answer << "CH# <" << ch_nr << ">";
  if (nr_parameters > 0) {
    answer << ", Parameters <"; 
    answer<< par_value[0];
    for (auto i=1; i<=nr_parameters; ++i) {
       answer << ", " << par_value[i];
    }
    answer << ">";
  }
  return answer.str();
}

std::string conf_reply_msg::toStr() {
  std::string s1, *s2;
  if (!handler_to_str(h_type, &s2)) {
    return "unknown handler type";
  }
  std::ostringstream answer;
  answer << "handler type <";
  answer << s2->c_str() << ">, ";
  answer << "log format <";
  answer << (format_to_str(log_f, &s1) ? s1 : "Unknown") << ">";
  if (nr_parameters > 0) {
    for (int i=0; i<nr_parameters; i++) {
      answer << ", arg. #" << i << ": " << par_value[i];
    }
  }
  return answer.str();
}
