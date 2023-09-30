#include "time_tsc.h"

#include "fox_end_handler.h"
#include "log.h"
#include "handlers/defect_handler.h"
#include "config_server/net_conf.h"

extern "C" {
#include "util.h"
}
#include <unistd.h> 
#include <zmq.hpp>
#include <string>
#include <iostream>
#include "globals.h"
#include <sstream>


struct alignas(CACHELINE_SIZE) foxend_closure : closure {
  zmq::socket_t* socket;
  zmq::context_t* context;
};

void FoxEndHandler(void * cl, int64_t tuple) {
  foxend_closure * ch_cl = (foxend_closure *) cl;

  int rc;
  struct timespec ts, ts2;
  
  // send message to the start node
  zmq::message_t request(sizeof(int64_t));
  zmq::message_t received(sizeof(int64_t));
  memcpy(request.data(), &tuple, sizeof(int64_t));

  clock_gettime_tsc(CLOCK_REALTIME, &ts);
  
  rc = ch_cl->socket->send (request);
  if (rc < 0) {
    logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
    exit(EXIT_FAILURE);
  }
  
  rc = ch_cl->socket->recv (&received);
  if (rc < 0) {
    logFail(__FILE__, __LINE__, "Error in ZMQ socket receive", rc);
    exit(EXIT_FAILURE);
  }

  clock_gettime_tsc(CLOCK_REALTIME, &ts2);

  int64_t rtt_ns = ((ts2.tv_sec * NANO_S_PER_S) + ts2.tv_nsec)
                   - ((ts.tv_sec * NANO_S_PER_S) + ts.tv_nsec);

  ch_cl->ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec
                          << ", " << ts.tsc
                          << ", " << rtt_ns << std::endl;
}

int setArgFoxEndHandler(void * cl, int32_t nr_par, int64_t par) {
  return 0;
}


closure * initFoxEndHandler(std::string ch_msg, int32_t mychannel, handler_type *ch_h, int64_t *par, log_format * f) {
  struct drand48_data dr48;
  int result;

  result = init_rand(&dr48);
  if (-1 == result) {
    logFail(__FILE__, __LINE__, "Error in initializing drand48", result);
    exit(EXIT_FAILURE);
  }
  char identity[10] = {};
  sprintf(identity, "%04X-%04X", within(&dr48, 0x10000), within(&dr48, 0x10000));

  // retry when config server is waiting for fox start node
  // par[0] == 1 means retry
  int index = 0;
  bool ret = true;
  while (par[0] == 0 && index < FOX_END_RETRY) {
    sleep(FOX_END_SLEEP);
    ret = getConfigFromServer(ch_msg, mychannel, ch_h, par, f);
    index++;
    ret = ret && (par[0] != -1);
  }

  if (!ret) {
    std::ostringstream err_msg;
    err_msg << "Ch: " << ch_msg;
    err_msg << ", ch_nr " << mychannel;
    err_msg << ": fox end unable to obtain configuration from server";
    logFail(__FILE__, __LINE__, err_msg.str());
    return initDefectHandler(ch_msg);
  }

  std::ostringstream addr;
  addr << "tcp://" << par[1] << "." << par[2] << "." << par[3] << "." 
       << par[4] << ":" << par[5];

  foxend_closure * tmp = new foxend_closure();
  tmp->handler_f = FoxEndHandler;
  tmp->parm_handler_f = setArgFoxEndHandler;

  tmp->context = new zmq::context_t(1);
  tmp->socket = new zmq::socket_t(*tmp->context, ZMQ_DEALER);
  tmp->socket->setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  tmp->socket->connect(addr.str());

//tmp->context = new zmq::context_t(2);
//tmp->socket = new zmq::socket_t(*tmp->context, ZMQ_PUSH);
//tmp->socket->connect(addr.str());

//ch_cl->ch_logf << "# tuple, " << std::endl;
  return tmp;
}

