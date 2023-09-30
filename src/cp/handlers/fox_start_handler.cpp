#include "time_tsc.h"

#include "globals.h"
#include "fox_start_handler.h"
#include "fox_end_handler.h"
#include "log.h"
#include "handlers/defect_handler.h"
#include "config_server/net_conf.h"
#include "time_io.h"
extern "C" {
#include "net.h"
}
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <zmq.hpp>
#include <string>
#include <sstream>
#include <vector>

void *FoxStartReceiver(void *data){
  closure * ch_cl = (closure *)data;

  /* Listening msg from fox ends */
  std::ostringstream ep;
  ep << "tcp://*:" << fox_start_default_port;

  zmq::context_t context (1);
  zmq::socket_t sock (context, ZMQ_ROUTER);

  sock.bind(ep.str());

  zmq::message_t identity;
  zmq::message_t zmqmsg;
//zmq::context_t context(2);
//zmq::socket_t socket(context, ZMQ_PULL);
//std::string addr_listen = "tcp://*:" + std::to_string(fox_start_default_port);
//socket.bind(addr_listen.c_str());

//zmq::message_t received(sizeof(int64_t));
  int64_t tuple;
  struct timespec ts;
  int rc;
  while (true)
  {
    rc = sock.recv (&identity);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket receive", rc);
      exit(EXIT_FAILURE);
    }
    rc = sock.recv (&zmqmsg);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket receive", rc);
      exit(EXIT_FAILURE);
    }
    clock_gettime_tsc(CLOCK_REALTIME, &ts);
    rc = sock.send(identity, ZMQ_SNDMORE);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
      exit(EXIT_FAILURE);
    }
    rc = sock.send(zmqmsg);
    if (rc < 0) {
      logFail(__FILE__, __LINE__, "Error in ZMQ socket send", rc);
      exit(EXIT_FAILURE);
    }
    memcpy(&tuple, zmqmsg.data(), sizeof(int64_t));
  //socket.recv(&received); // wait start node
  //clock_gettime_tsc(CLOCK_REALTIME, &ts);
  //memcpy(&tuple, received.data(), sizeof(int64_t));
    ch_cl->ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec 
                            << ", " << ts.tsc << std::endl;
  }

  return 0;
}

void FoxStartHandler(void * cl, int64_t tuple) {
  closure * ch_cl = (closure *) cl;
  struct timespec ts;
  clock_gettime_tsc(CLOCK_REALTIME, &ts);
  ch_cl->ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec
                          << ", " << ts.tsc << std::endl;
}

closure * initFoxStartHandler(std::string ch_msg, int32_t mychannel, handler_type *ch_h, int64_t *par, log_format * f) {

  char tmpAddr[255];
  getIPDottedQuad(tmpAddr, 255);
  std::vector<int> addr_vector = split_ip(std::string(tmpAddr),'.');
  par[0] = 5;
  for (int i = 1; i < 5; i++) {
    par[i] = addr_vector[i-1];
  }
  par[5] = fox_start_default_port;
  bool ret = getConfigFromServer(ch_msg, mychannel, ch_h, par, f);

  if (!ret) {
    std::ostringstream err_msg;
    err_msg << "Ch: " << ch_msg;
    err_msg << ", ch_nr " << mychannel;
    err_msg << ": unable to obtain configuration from server";
    logFail(__FILE__, __LINE__, err_msg.str());
    return initDefectHandler(ch_msg);
  }

  closure * tmp = new closure();
  tmp->handler_f = FoxStartHandler;
  tmp->parm_handler_f = NULL;

  // listening thread
  pthread_t thread;
  int thr_id;

  thr_id = pthread_create(&thread, NULL, FoxStartReceiver, (void *)tmp);
  if (thr_id < 0){
    std::ostringstream err_msg;
    err_msg << "Ch: " << ch_msg;
    err_msg << ", ch_nr " << mychannel;
    err_msg << "failed to create thread";
    logFail(__FILE__, __LINE__, err_msg.str());
    exit(0);
  }

  return tmp;
}

std::vector<int> split_ip(const std::string& str, char delim) {
    std::vector<int> words;
    std::string word;
    std::istringstream stream(str);
    while (getline(stream, word, delim)) {
        words.push_back(std::atoi(word.c_str()));
    }
    return words;
}
