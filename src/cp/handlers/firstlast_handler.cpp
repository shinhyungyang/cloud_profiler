#include "time_tsc.h"

#include "firstlast_handler.h"
#include "null_handler.h"
#include "log.h"
#include "globals.h"

#include <stdlib.h>

struct alignas(CACHELINE_SIZE) firstlast_closure : closure {
  bool has_first;
  int64_t tuple;
  struct timespec ts;
  virtual void closeChannel(void);
};

void firstlast_closure::closeChannel(void) {
  if (!has_first) {
    // No tuple has been logged:
    ch_logf << "ERROR: no tuple logged!" << std::endl;
  } else {
    // Log the last tuple (which might be the first tuple):
    ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec 
                     << ", " << ts.tsc << std::endl;
  }
  closure::closeChannel();
}

void FirstlastHandler(void * cl, int64_t tuple) {
  firstlast_closure * fcl = (firstlast_closure *) cl;
  fcl->tuple = tuple;
  clock_gettime_tsc(CLOCK_REALTIME, &fcl->ts);
  if (!fcl->has_first) {
    fcl->has_first = true;
    // Initial log:
    fcl->ch_logf << tuple << ", " << fcl->ts.tv_sec << ", "
                 << fcl->ts.tv_nsec << ", " << fcl->ts.tsc << std::endl;
  }
}

closure * initFirstlastHandler(log_format format) {
  firstlast_closure * fcl = new firstlast_closure();
  fcl->handler_f = FirstlastHandler;
  fcl->parm_handler_f = NULL;
  fcl->has_first = false;
  fcl->tuple = -1;
  return fcl;
}
