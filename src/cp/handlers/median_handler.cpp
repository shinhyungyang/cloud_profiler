#include "time_tsc.h"

#include "median_handler.h"
#include "null_handler.h"
#include "log.h"
#include "globals.h"

#include <stdlib.h>

static inline int64_t roundCachlineSize(int64_t size);
static inline int void_compareTS(const void * t1, const void * t2);

struct alignas(CACHELINE_SIZE) median_closure : closure {
  struct timespec * buffer;
  int64_t nr_ts;  // current number of stored time-stamps
  int64_t max_ts; // Max number of ts due to internal buffer size; Note that
                  // this number can change due to parameterization.
};

void MedianHandler(void * cl, int64_t tuple) {
  median_closure * mcl = (median_closure *) cl;
  mcl->nr_ts++;
  if(mcl->nr_ts < mcl->max_ts) {
    // Get timestamp and store in buffer:
    clock_gettime_tsc(CLOCK_REALTIME, mcl->buffer + mcl->nr_ts);
  } else {
    // Compute median and write to logfile:
    // FIXME: This could be improved using the C++ nth_element() method.
    int64_t max_ts = mcl->max_ts;
    qsort(mcl->buffer, max_ts, sizeof(struct timespec), void_compareTS); 
    struct timespec * resPtr, sum, res;
    if(max_ts % 2 == 0) {
      // Even number of elements, average the 2 middle elements:
      addTS(&sum, &mcl->buffer[(max_ts-1)/2], &mcl->buffer[max_ts/2]);
      divideTS(&res, &sum, 2); 
      resPtr = &res;
    } else {
      // Odd number of elements:
      resPtr = &mcl->buffer[max_ts/2]; 
    }
    mcl->ch_logf << resPtr->tv_sec << ", " << resPtr->tv_nsec
                                   << ", " << resPtr->tsc << std::endl;
    mcl->nr_ts = 0;
  }
}

closure * initMedianHandler(log_format format) {
  median_closure * mcl = new median_closure();
  int64_t size = roundCachlineSize(MAX_TS * sizeof(struct timespec));
  mcl->buffer = (struct timespec *) aligned_alloc(CACHELINE_SIZE, size);
  if (mcl->buffer == NULL) {
    mcl->handler_f = NullHandler;
    mcl->parm_handler_f = NULL;
    mcl->nr_ts = 0;
    mcl->max_ts = 0;
    logFail(__FILE__, __LINE__, "initMedianHandler aligned alloc");
  } else {
    mcl->handler_f = MedianHandler;
    mcl->parm_handler_f = setArgMedianHandler;
    mcl->nr_ts = 0;
    mcl->max_ts = MAX_TS; 
  }
  return mcl;
}

int setArgMedianHandler(void * cl, int32_t arg_number, int64_t arg_value) {
  median_closure * mcl = (median_closure *) cl;
  if ((arg_number != 0) || (arg_value < (int64_t)sizeof(struct timespec))) {
    logFail(__FILE__, __LINE__, "setArgMedianHandler wrong arguments");
    mcl->closeChannel(); 
    return -1;
  }
  int64_t size = roundCachlineSize(arg_value * sizeof(struct timespec));
  struct timespec * tmp = (struct timespec *)realloc (mcl->buffer, size);
  if (((uint64_t)tmp)%CACHELINE_SIZE == 0) {
     // Memory buffer is still aligned:
     mcl->buffer = tmp; 
  } else {
     // Memory not aligned anymore, need to free and re-alloc:
     free(tmp);
     mcl->buffer = (struct timespec *) aligned_alloc(CACHELINE_SIZE, size);
     if (mcl->buffer == NULL) {
        mcl->handler_f = NullHandler;
        mcl->parm_handler_f = NULL;
        mcl->nr_ts = 0;
        mcl->max_ts = 0;
        logFail(__FILE__, __LINE__, "setArgMedianHandler re-alloc");
        return -1;
      }
  }
  mcl->nr_ts = 0;
  mcl->max_ts = arg_value; 
  return 0; 
}

static int64_t roundCachlineSize(int64_t size) {
  return size%CACHELINE_SIZE==0?size:((size/CACHELINE_SIZE)+1)*CACHELINE_SIZE;
}

static inline int void_compareTS(const void * t1, const void * t2) {
  return compareTS((struct timespec *) t1, (struct timespec *) t2); 
}
