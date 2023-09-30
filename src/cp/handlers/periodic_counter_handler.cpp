#include "time_tsc.h"

#include "periodic_counter_handler.h"
#include "null_handler.h"
#include "log.h"
#include "globals.h"

#include <vector>
#include <iostream>

#include <stdlib.h>
#include <math.h>
#include <assert.h>

struct PC_closure;
static void printCtr(PC_closure * PCcl);
static void printInitialHeaderAndPeriod(PC_closure * PCcl);
static void printPeriod(PC_closure * PCcl);
static void printSkippedIntervals(PC_closure * PCcl, uint32_t intervals);
static void flushCtr(PC_closure * PCcl, struct timespec * now);
static void getNextShot_and_flushCtr(PC_closure * PCcl,
                                     struct timespec * next_shot,
                                     struct timespec * now);

struct alignas(CACHELINE_SIZE) PC_closure : closure {
  uint64_t ctr;
  struct timespec time_start;
  struct timespec time_flush;
  struct timespec periodic_inc;
  uint64_t periodic_inc_ns; // alternative presentation to save CPU cycles
  bool isHeaderLogged = false;
  virtual void closeChannel(void);
};

void PC_closure::closeChannel(void) {
  char buf[1024];
  struct timespec now;
  if (this->isHeaderLogged == false) {
    printInitialHeaderAndPeriod(this);
  }
  clock_gettime_tsc(CLOCK_REALTIME, &now);
  flushCtr(this, &now);
  formatTS(&now, buf, 1024);
  this->ch_logf << buf << ", *** timer shut down ***" << std::endl;
  closure::closeChannel();
}

void PeriodicCounterHandler(void * cl, int64_t tuple) {
  PC_closure * PCcl = (PC_closure *) cl;
  struct timespec now;
  clock_gettime_tsc(CLOCK_REALTIME, &now);
  if (now.tv_sec > PCcl->time_flush.tv_sec ||
       (now.tv_sec == PCcl->time_flush.tv_sec
         && now.tv_nsec > PCcl->time_flush.tv_nsec)) {
    if (PCcl->isHeaderLogged == false) {
      printInitialHeaderAndPeriod(PCcl);
    }
    // Compute time of next shot and flush the counter:
    struct timespec next_shot;
    getNextShot_and_flushCtr(PCcl, &next_shot, &now); 
    // Set time for next flush:
    addTS(&PCcl->time_flush, &PCcl->time_flush, &next_shot);
    // Initialize counter for next period:
    PCcl->ctr = 1;
  } else {
    PCcl->ctr++;
  }
}

closure * initPeriodicCounterHandler(log_format format) {
  PC_closure * PCcl = new PC_closure();
  PCcl->handler_f = PeriodicCounterHandler;
  PCcl->parm_handler_f = setArgPeriodicCounterHandler;
  PCcl->ctr = 0;
  // Set the counter's period (periodic_inc):
  PCcl->periodic_inc.tv_sec  = DEFAULT_PERIOD / NANO_S_PER_S;
  PCcl->periodic_inc.tv_nsec = DEFAULT_PERIOD % NANO_S_PER_S; 
  PCcl->periodic_inc_ns      = DEFAULT_PERIOD; // Alternate representation
  // Set the time for the initial flush of the counter:
  clock_gettime_tsc(CLOCK_REALTIME, &PCcl->time_start);
  addTS(&PCcl->time_flush, &PCcl->time_start, &PCcl->periodic_inc);
  return PCcl;
}

int setArgPeriodicCounterHandler(void * cl,
                                 int32_t arg_number,
                                 int64_t arg_value) {
  PC_closure * PCcl = (PC_closure *) cl;
  if (arg_number != 0) {
    logFail(__FILE__, __LINE__,
            "setArgPeriodicCounterHandler arg number out of range");
    PCcl->closeChannel(); 
    return -1;
  }
  if (arg_value < 0) {
    logFail(__FILE__, __LINE__,
            "setArgPeriodicCounterHandler arg 0 val out of range");
      PCcl->closeChannel(); 
      return -1;
  }
  if (PCcl->isHeaderLogged == false) {
    printInitialHeaderAndPeriod(PCcl);
  }
  clock_gettime_tsc(CLOCK_REALTIME, &PCcl->time_start);
  // Flush previous counter interval:
  flushCtr(PCcl, &PCcl->time_start);
  // Set the counter's new period (periodic_inc):
  PCcl->periodic_inc.tv_sec  = arg_value / NANO_S_PER_S;
  PCcl->periodic_inc.tv_nsec = arg_value % NANO_S_PER_S; 
  PCcl->periodic_inc_ns      = arg_value; // Alternate representation
  // Set the time for the next flush of the counter:
  addTS(&PCcl->time_flush, &PCcl->time_start, &PCcl->periodic_inc);
  // Mark the reprogramming event in the log-file:
  char buf[1024];
  formatTS(&PCcl->time_start, buf, 1024);
  PCcl->ch_logf << buf << ", *** timer reprogrammed ***" << std::endl;
  printPeriod(PCcl);
  // Reset the counter value:
  PCcl->ctr = 0;
  return 0; 
}

static void printCtr(PC_closure * PCcl) {
  // Flush regular period's counter value:
  char buf[1024];
  formatTS(&PCcl->time_flush, buf, 1024);
  PCcl->ch_logf << buf << ", " << PCcl->ctr << std::endl; 
}

static void printInitialHeaderAndPeriod(PC_closure * PCcl) {
  PCcl->ch_logf << "# <end_of_period>, <number_of_events>" << std::endl;
  printPeriod(PCcl);
  PCcl->isHeaderLogged = true;
}

static void printPeriod(PC_closure * PCcl) {
  char buf[1024];
  formatTS(&PCcl->time_start, buf, 1024);
  PCcl->ch_logf << buf << ", start, period: "; 
  formatDuration(&PCcl->periodic_inc, buf, 1024, true);
  PCcl->ch_logf << buf << std::endl; 
}

static void printSkippedIntervals(PC_closure * PCcl, uint32_t intervals) {
  PCcl->ch_logf << "# *** skipped " << intervals << " intervals ***";
  PCcl->ch_logf << std::endl;
}

static void flushCtr(PC_closure * PCcl, struct timespec * now) {
  // Compute number of skipped intervals. Depending on the
  // application, we might have skipped already >1 intervals:
  bool truncated = false;
  uint32_t skipped_intervals_total;
  char buf[1024];
  if (-1 == compareTS(now, &PCcl->time_flush)) {
    // The timer has not expired yet.
    truncated = true;
    skipped_intervals_total = 0;
  } else {
    // The timer has expired already.
    struct timespec elapsed;
    subtractTS(&elapsed, &PCcl->time_flush, now); 
    uint64_t elapsed_ns = elapsed.tv_sec * NANO_S_PER_S + elapsed.tv_nsec; 
    skipped_intervals_total = elapsed_ns / PCcl->periodic_inc_ns;
    if (PCcl->ctr == 0) {
      // We additionally skipped the first interval.
      // (Up to here, skiped_intervals_total counts the skipped intervals
      //  past the first one.)
      skipped_intervals_total++;
    }
  }
  if (truncated) {
    // Flush truncated period's counter value:
    formatTS(now, buf, 1024);
    PCcl->ch_logf << buf << ", " << PCcl->ctr;
    PCcl->ch_logf << " <truncated period>" << std::endl; 
  } else if (PCcl->ctr > 0) {
    printCtr(PCcl);
  }
  if (skipped_intervals_total > 0) {
    printSkippedIntervals(PCcl, skipped_intervals_total);
  }
}

static void getNextShot_and_flushCtr(PC_closure * PCcl,
                                     struct timespec * next_shot,
                                     struct timespec * now)
{
  // Compute time for the next flush of the counter. Depending on the
  // application, we might have skipped already >1 intervals:
  uint64_t next_shot_ns;
  uint32_t skipped_intervals_total;
  struct timespec elapsed;
  subtractTS(&elapsed, &PCcl->time_flush, now); 
  uint64_t elapsed_ns = elapsed.tv_sec * NANO_S_PER_S + elapsed.tv_nsec; 
  uint32_t skipped_intervals_from_flush = elapsed_ns / PCcl->periodic_inc_ns;
  next_shot_ns = (1 + skipped_intervals_from_flush) * PCcl->periodic_inc_ns;
  next_shot->tv_sec  = next_shot_ns / NANO_S_PER_S;
  next_shot->tv_nsec = next_shot_ns % NANO_S_PER_S;
  if (PCcl->ctr > 0) {
    printCtr(PCcl);
  }
  if (PCcl->ctr == 0) {
    // We additionally skipped the first interval.
    // (skiped_intervals_from_flush counts the skipped intervals
    //  past the first one.)
    skipped_intervals_total = skipped_intervals_from_flush + 1;
  } else {
    skipped_intervals_total = skipped_intervals_from_flush;
  }
  if (skipped_intervals_total > 0) {
    printSkippedIntervals(PCcl, skipped_intervals_total);
  }
}
