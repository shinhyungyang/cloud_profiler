#include "time_tsc.h"

#include "downsample_handler.h"
#include "null_handler.h"
#include "globals.h"
#include "log.h"

#include <stdlib.h>

struct alignas(CACHELINE_SIZE) downsample_closure : closure {
  int64_t factor;  // downsampling factor
  int64_t counter; // sample counter
};

void DownsampleHandler(void * cl, int64_t tuple) {
  downsample_closure * dcl = (downsample_closure *) cl;
  if (dcl->counter < dcl->factor) {
    dcl->counter++;
  } else {
    struct timespec ts;
    clock_gettime_tsc(CLOCK_REALTIME, &ts);
    dcl->ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec 
                          << ", " << ts.tsc << std::endl;
    dcl->counter = 1;
  }  
}

closure * initDownsampleHandler(log_format format) {
  downsample_closure * dcl = new downsample_closure();
  dcl->handler_f = DownsampleHandler;
  dcl->parm_handler_f = setArgDownsampleHandler;
  dcl->factor = SAMPLING_FACTOR;
  dcl->counter = 1;
  return dcl;
}

int setArgDownsampleHandler(void * cl, int32_t arg_number, int64_t arg_value) {
  downsample_closure * dcl = (downsample_closure *) cl;
  if ((arg_number != 0) && (arg_number != -1)) {
    logFail(__FILE__, __LINE__,
            "setArgDownsampleHandler arg number out of range");
    dcl->closeChannel(); 
    return -1;
  }
  if (arg_number == -1) {
    // Set 'all' arguments at once:
    uint64_t * par = reinterpret_cast<uint64_t *>(arg_value);
    if (par[0] != 1) {
      logFail(__FILE__, __LINE__,
              "setArgDownsampleHandler arg number out of range");
      dcl->closeChannel(); 
      return -1;
    }
    return setArgDownsampleHandler(cl, 0, par[1]);
  }
  if ((arg_number != 0) && (arg_value < 1)) {
    logFail(__FILE__, __LINE__, "setArgDownsampleHandler argument error");
    dcl->closeChannel(); 
    return -1;
  }
  dcl->factor = arg_value;
  return 0; 
}
