#include "time_tsc.h"

#include "XoY_handler.h"
#include "null_handler.h"
#include "log.h"
#include "globals.h"

#include <stdlib.h>
#include <math.h>

struct alignas(CACHELINE_SIZE) XoY_closure : closure {
  uint64_t x_factor;
  uint64_t y_factor;
  uint64_t y_mask;
};

static bool checkConsistency(uint64_t X, uint64_t Y);
// Check the consistency of the X and Y parameter values.

void XoYHandler(void * cl, int64_t tuple) {
  XoY_closure * XoYcl = (XoY_closure *) cl;
  if ((((uint64_t) tuple) & XoYcl->y_mask) < XoYcl->x_factor) { 
    struct timespec ts;
    clock_gettime_tsc(CLOCK_REALTIME, &ts);
    XoYcl->ch_logf << tuple << ", " << ts.tv_sec << ", " << ts.tv_nsec
                            << ", " << ts.tsc << std::endl;
  }  
}

closure * initXoYHandler(log_format format) {
  XoY_closure * XoYcl = new XoY_closure();
  XoYcl->handler_f = XoYHandler;
  XoYcl->parm_handler_f = setArgXoYHandler;
  XoYcl->x_factor = X_FACTOR;
  XoYcl->y_factor = Y_FACTOR;
  XoYcl->y_mask = Y_FACTOR - 1;
  return XoYcl;
}

int setArgXoYHandler(void * cl, int32_t arg_number, int64_t arg_value) {
  XoY_closure * XoYcl = (XoY_closure *) cl;
  if ((arg_number != 0) && (arg_number != 1) && (arg_number != -1)) {
    logFail(__FILE__, __LINE__, "setArgXoYHandler arg number out of range");
    XoYcl->closeChannel(); 
    return -1;
  }
  if (arg_number == -1) {
    // Setting all arguments at once:
    uint64_t * par = reinterpret_cast<uint64_t *>(arg_value);
    if (par[0] != 2) {
      logFail(__FILE__, __LINE__,
              "setArgXoYHandler arg number out of range");
      return -1;
    }
    // XoY arg order: arg 1 -> arg 0 (see comments in header):
    int ret = setArgXoYHandler(cl, 1, par[2]);
    if (ret == -1)
      return -1;
    return setArgXoYHandler(cl, 0, par[1]);
  }
  if (arg_number == 0) {
    if (checkConsistency(arg_value, XoYcl->y_factor)) {
      XoYcl->x_factor = arg_value;
    } else {
      logFail(__FILE__, __LINE__, "setArgXoYHandler arg 0 inconsistent");
      XoYcl->closeChannel(); 
      return -1;
    }
  }
  if (arg_number == 1) {
    if (checkConsistency(XoYcl->x_factor, arg_value)) {
      XoYcl->y_factor = arg_value;
      XoYcl->y_mask = arg_value - 1;
    } else {
      logFail(__FILE__, __LINE__, "setArgXoYHandler arg 1 inconsistent");
      XoYcl->closeChannel(); 
      return -1;
    }
  }
  return 0; 
}

static bool checkConsistency(uint64_t X, uint64_t Y) {
  if (Y < X)
    return false;
  double param = (double)Y;
  double result = floor(log2(param));
  if (pow(2, result) != (double)Y)
    return false;
  return true;
}
