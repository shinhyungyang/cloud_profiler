#include "time_tsc.h"

#include "fast_periodic_counter_handler.h"
#include "log.h"
#include "time_io.h"
#include "globals.h"

#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <math.h>
#include <assert.h>

static std::atomic<uint32_t> nr_open_channels(0);
// Reference counter to detect when to shut down the FPC_DEFAULT_PERIOD
// sampling thread. Every time this counter reaches zero in closeChannel(),
// shut down will be conducted. All other sampling threads shut down with the
// last channel that is released from a sampling thread.
 

struct FPC_closure;
struct thr_descriptor;
struct job;
static void printTimeStamp(FPC_closure * FPCcl);
static void printCtrStart(FPC_closure * FPCcl, uint64_t ctr);
static void printTruncatedPeriod(FPC_closure * FPCcl, uint64_t ctr,
                                 std::string msg);
static void printInitialHeaderAndPeriod(FPC_closure * FPCcl);
static void printSkippedIntervals(FPC_closure * cl);
static void printTotalCount(FPC_closure * cl);
static void enqueueJobWithThrDescriptor(job * jb);
static void dequeueJobFromThrDescriptor(job * jb);
static bool removeThrDescriptor (uint64_t period);

//===----------------------------------------------------------------------===//
//             FPC_closure struct definition and related functions
//===----------------------------------------------------------------------===//
//
struct alignas(CACHELINE_SIZE) FPC_closure : closure {
  std::atomic<uint64_t> ctr;
  uint64_t sampled_ctr __attribute__((aligned(CACHELINE_SIZE)));
  struct timespec sampled_ts;   // time this channel ctr got sampled
  uint64_t total_ctr;
  uint64_t periodic_inc_ns;
  struct timespec periodic_inc; // alternate representation for fast I/O
  uint64_t skipped_intervals;
  bool isHeaderLogged;
  bool isRefCounted;            // True if channel included in nr_open_channels
  virtual bool openChannel(int32_t ch_nr, std::string ch_msg, log_format f);
  virtual void closeChannel();
};

void FastPeriodicCounterHandler(void * cl, int64_t tuple) {
  FPC_closure * FPCcl = (FPC_closure *) cl;
  FPCcl->ctr.fetch_add(1, std::memory_order_acq_rel);
}

//===----------------------------------------------------------------------===//
//                           job_queue class definitions
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Implementation of the job and job_queue structs. Each sampling thread
// maintains one job queue for inserting, moving or deleting channels from this
// sampling thread. Job queues are thus thread-safe, supporting multiple
// producers.  Changing the period of channel c's counter from x to y requires
// the removal of c from thread x and inserting c at thread y. This is
// accomplished by a MOVE_CHANNEL job. 
//
enum class job_type {INSERT_CHANNEL, MOVE_CHANNEL, DELETE_CHANNEL}; 

struct job {
  job_type type;
  FPC_closure * FPCcl;

  uint64_t cur_periodic_inc_ns;
  // Current channel's target period. This is only used if the desired target
  // period is different from the FPCcl's period. I.e., in case of a
  // MOVE_CHANNEL job.
 
  job(job_type type, FPC_closure * cl, uint64_t target_period = -1);
  // The default-argument applies if target_period is the same as the period
  // from the FPCcl.


  std::mutex mtx_syncAPI;
  std::condition_variable cond_syncAPI;
  bool jobCompleted;
  // Jobs are asynchronous. They are completed once the sampling thread has
  // processed them. API calls submitting jobs are therefore required to
  // synchronize on the job completion, using the above condition variable.

  // The following API calls use jobs and hence the below methods for
  // synchronization:
  // - FPC_closure::openChannel()
  // - FPC_closure::closeChannel()
  // - setArgFastPeriodicCounterHandler()
  void waitCompletion();
  void signalCompletion();
};

job::job(job_type type, FPC_closure * cl, uint64_t target_period) {
  this->type = type;
  jobCompleted = false;
  FPCcl = cl;
  cur_periodic_inc_ns = target_period;
}

void job::waitCompletion() {
  // Wait for completion:
  std::unique_lock<std::mutex> mlock(mtx_syncAPI); // mutex scope lock
  while (!jobCompleted) { // guard against spurious wakeups
    cond_syncAPI.wait(mlock);
  }
}

void job::signalCompletion() {
  std::unique_lock<std::mutex> mlock(mtx_syncAPI); // mutex scope lock
  jobCompleted = true;
  mlock.unlock();
  cond_syncAPI.notify_one();
}

struct job_queue {
  std::mutex mtx_q;
  std::queue<struct job *> job_queue;
  void add_job (job * j);   // thread-safe enqueue
  job * remove_job (void);  // thread-safe dequeue
  std::queue<struct job *>::size_type size(void); // thread-safe size
};

void job_queue::add_job(job *j) {
  mtx_q.lock();
  job_queue.push(j);
  mtx_q.unlock();
}

job * job_queue::remove_job(void) {
  mtx_q.lock();
  auto ret = job_queue.front();
  job_queue.pop();
  mtx_q.unlock();
  return ret;
}

std::queue<struct job *>::size_type job_queue::size(void) {
  mtx_q.lock();
  auto ret = job_queue.size();
  mtx_q.unlock();
  return ret;
}

//===----------------------------------------------------------------------===//
//                    thr_descriptor struct definition
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Implementation of the sampling threads' thr_descriptor struct.
//
struct thr_descriptor {
  std::map<int32_t, struct FPC_closure *> closures; // map: CH# -> closure *
  uint64_t periodic_inc_ns;
  struct timespec periodic_inc; // alternate representation
  struct job_queue j_queue;
  std::thread sampling_thr;
  void thr_f();
  thr_descriptor(job * jb);
  void process_job_queue();       // handle jobs for this sampling thread
  bool dequeue_channel(job * jb); // remove channel from sampling thread
  bool shallTerminate();          // sampling thread termination check
};

thr_descriptor::thr_descriptor(job * jb) {
  this->periodic_inc_ns = jb->FPCcl->periodic_inc_ns;
  periodic_inc.tv_sec  = periodic_inc_ns / NANO_S_PER_S;
  periodic_inc.tv_nsec = periodic_inc_ns % NANO_S_PER_S;
  j_queue.add_job(jb);
  sampling_thr = std::thread(&thr_descriptor::thr_f, this); 
  sampling_thr.detach(); // no synchronization on sampling thread termination
}

bool thr_descriptor::dequeue_channel(job * jb) {
  auto it = closures.find(jb->FPCcl->ch_nr);
  char buf[1024];
  if (it == closures.end()) {
    formatDuration(&periodic_inc, buf, 1024, true);
    std::ostringstream err_msg;
    err_msg << "Ch: " << jb->FPCcl->ch_nr;
    err_msg << " not part of sampling thread for period " << buf;
    logFail(__FILE__, __LINE__, err_msg.str());
    return false;
  } else {
    closures.erase(it); // Remove from this thread's descriptor queue 
    return true;
  }
}

void thr_descriptor::process_job_queue() {
  uint64_t ev_count;
  std::map<int32_t, struct FPC_closure *>::iterator it;
  std::queue<struct job *>::size_type nr_jobs = j_queue.size(); 
  for (std::queue<struct job *>::size_type j = 0; j < nr_jobs; j++) {
    job * jb = j_queue.remove_job(); 
    auto FPCcl = jb->FPCcl;
    switch (jb->type) {
      case job_type::INSERT_CHANNEL:
        if (!FPCcl->isHeaderLogged) {
          FPCcl->isHeaderLogged = true;
          printInitialHeaderAndPeriod(FPCcl);
        }
        ev_count = FPCcl->ctr.exchange(0, std::memory_order_acq_rel);
        FPCcl->total_ctr += ev_count;
        printCtrStart(FPCcl, ev_count);
        closures.insert({ FPCcl->ch_nr, FPCcl });
        jb->signalCompletion();
        break;
      case job_type::MOVE_CHANNEL:
        if (this->dequeue_channel(jb)) {
          ev_count = FPCcl->ctr.exchange(0, std::memory_order_acq_rel);
          FPCcl->total_ctr += ev_count;
          if (FPCcl->skipped_intervals > 0) {
            printSkippedIntervals(FPCcl);
            FPCcl->skipped_intervals = 0;
          }
          printTruncatedPeriod(FPCcl, ev_count, "timer reprogrammed");
          jb->type = job_type::INSERT_CHANNEL;
          FPCcl->periodic_inc_ns = jb->cur_periodic_inc_ns;
          FPCcl->periodic_inc.tv_sec = FPCcl->periodic_inc_ns / NANO_S_PER_S;
          FPCcl->periodic_inc.tv_nsec = FPCcl->periodic_inc_ns % NANO_S_PER_S;
          jb->cur_periodic_inc_ns = -1; // not relevant because FPCcl updated 
          // Enqueue with target thread's descriptor queue:
          enqueueJobWithThrDescriptor(jb);
        } else {
          // Error case (error logged with dequeue_channel()):
          jb->signalCompletion();
        }
        break;
      case job_type::DELETE_CHANNEL:
        // Flush counter and release channel from sampling thread:
        ev_count = FPCcl->ctr.exchange(0, std::memory_order_acq_rel);
        FPCcl->total_ctr += ev_count;
        if (FPCcl->skipped_intervals > 0) {
            printSkippedIntervals(FPCcl);
            FPCcl->skipped_intervals = 0;
        }
        printTruncatedPeriod(FPCcl, ev_count, "timer shut down");
        printTotalCount(FPCcl);
        this->dequeue_channel(jb);
        jb->signalCompletion();
      break;
    }
  }
}

bool thr_descriptor::shallTerminate() {
  // The FPC_DEFAULT_PERIOD sampling thread continues until the open channel's
  // reference counter drops to zero. All other sampling threads terminate
  // when the last closure is removed AND the job-queue is empty.
  //
  // Note: After calling this method, an opening channel might enqueue
  // at this thr_descriptor. Thus, the number of queued jobs must be
  // re-checked onece we hold the mutex of the thr_map. 
  if (periodic_inc_ns == FPC_DEFAULT_PERIOD && nr_open_channels == 0) {
    return true;
  } else if (j_queue.size() == 0 && closures.size() == 0) {
    return true;
  } else {
    return false;
  }
}

void thr_descriptor::thr_f() {
  //
  // This is the thread method that each sampling thread (part of a
  // thr_descriptor) is executing:
  //
  struct timespec ts_flush, ts_now, ts_sleep;
  bool terminate = false;
  bool doSnooze;

  if (periodic_inc_ns == NANO_S_PER_S) {
    clock_gettime_tsc(CLOCK_REALTIME, &ts_flush);
    ts_flush.tv_sec++;
    ts_flush.tv_nsec = 0;
  } else {
    clock_gettime_tsc(CLOCK_REALTIME, &ts_now); 
    addTS(&ts_flush, &ts_now, &periodic_inc);
  }
  while (!terminate) {
    // Sample all channels registered with this sampling thread:
    auto end_it = closures.end();
    for (auto it = closures.begin(); it != end_it; it++) {
      auto FPCcl = it->second;
      FPCcl->sampled_ctr = FPCcl->ctr.exchange(0, std::memory_order_acq_rel);
      clock_gettime_tsc(CLOCK_REALTIME, &FPCcl->sampled_ts);
    } 
    // Process channels' sampled data:
    for (auto it = closures.begin(); it != end_it; it++) {
      auto FPCcl = it->second;
      if (FPCcl->sampled_ctr == 0) {
        // No event on this channel during the past interval:
        FPCcl->skipped_intervals++;
      } else {
        FPCcl->total_ctr += FPCcl->sampled_ctr;
        if (FPCcl->skipped_intervals > 0) {
          printSkippedIntervals(FPCcl);
          FPCcl->skipped_intervals = 0;
        }
        printTimeStamp(FPCcl);
      }
    } 
    // Light sleep until ts_flush or thread runs out of channels to sample:
    doSnooze = true;
    while (doSnooze && !terminate) {
      // Process all new jobs submitted for this sampling thread:
      process_job_queue();
      if (shallTerminate() && removeThrDescriptor(periodic_inc_ns)) {
          // It was possible to remove descriptor from thr_map -> terminate
          terminate = true;
      } else {
        clock_gettime_tsc(CLOCK_REALTIME, &ts_now); 
        if (compareTS(&ts_now, &ts_flush) > 0) {
          // Flush is due, terminate sleeping loop:
          addTS(&ts_flush, &ts_flush, &periodic_inc);
          doSnooze = false; 
        } else {
          // Flush not due yet, continue sleep:
          subtractTS(&ts_sleep, &ts_now, &ts_flush);
          uint64_t sleep_ns = ts_sleep.tv_sec * NANO_S_PER_S + ts_sleep.tv_nsec;
          if (sleep_ns > DEFAULT_SNOOZE_PERIOD) {
            sleep_ns = DEFAULT_SNOOZE_PERIOD;
          }
          std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_ns));
        }
      }
    } // while (sleeping)
  }
  // Termination of this sampling thread (thr_descriptor).
  // Delete this thr_descriptor (the thread is detached):
  delete this;
}

//===----------------------------------------------------------------------===//
//                thr_map container definition and helper functions
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Map: sampling period (uint64_t) -> thread descriptor (thr_descriptor) 
//
// This is the container to keep thread descriptors. Thread descriptors are
// created when a job for a sampling period p is entered and no thread for
// period p exists yet.
// Sampling threads are detached and terminate if their queue of jobs becomes
// empty. The sampling thread for FPC_DEFAULT_PERIOD is an exception to this rule.
// Because openChannel() enqueues all channels with the FPC_DEFAULT_PERIOD
// sampling thread, it is more efficient to keep this sampling thread until
// the number of open channels reaches zero.

static std::unordered_map<uint64_t, thr_descriptor *> thr_map;
static std::mutex mtx_thr_map; // Guarding thr_map

static void enqueueJobWithThrDescriptor(job * jb) {
  uint64_t period = jb->FPCcl->periodic_inc_ns;
  mtx_thr_map.lock();
  auto it = thr_map.find(period);
  if (it == thr_map.end()) {
    // Create new sampling thread (descriptor):
    auto descr = new thr_descriptor(jb);
    // Store thread descriptor in thread map:
    thr_map.insert({ period, descr });
  } else {
    // Enter this channel in the job queue of the existing
    // sampling thread (descriptor):
    it->second->j_queue.add_job(jb);
  }
  mtx_thr_map.unlock();
}

static void dequeueJobFromThrDescriptor(job * jb) {
  char buf[1024];
  uint64_t period = jb->FPCcl->periodic_inc_ns;
  mtx_thr_map.lock();
  auto it = thr_map.find(period);
  if (it == thr_map.end()) {
    // No such thread:
    formatDuration(period, buf, 1024, true);
    std::ostringstream err_msg;
    err_msg << "Ch: " << jb->FPCcl->ch_nr;
          err_msg << " not part of sampling thread for period " << buf;
          logFail(__FILE__, __LINE__, err_msg.str());
  } else {
    // Remove this channel from the job queue of the existing
    // sampling thread (descriptor):
    it->second->j_queue.add_job(jb);
  }
  mtx_thr_map.unlock();
}

static bool removeThrDescriptor (uint64_t period) {
  mtx_thr_map.lock();
  auto it = thr_map.find(period);
  char buf[1024];
  if (it == thr_map.end()) {
    // No thread for asked period:
    formatDuration(period, buf, 1024, true);
    std::ostringstream err_msg;
    err_msg << "No thr_descriptor for period " << buf;
    logFail(__FILE__, __LINE__, err_msg.str());
    mtx_thr_map.unlock();
    return false;
  } 
  auto thr_descr = it->second;
  if (thr_descr->j_queue.size() > 0) {
    // Cannot remove thr_descriptor with job enqueued. This is not an error,
    // because a job could have been enqueued since the last check of the
    // queue:
    mtx_thr_map.unlock();
    return false;
  }
  if (thr_descr->closures.size() > 0) {
    // Closures remain. This is an error: 
    formatDuration(period, buf, 1024, true);
    std::ostringstream err_msg;
    err_msg << "Cannot delete thr_descriptor for period " << buf;
    err_msg << " because of remaining closures " << std::endl;
    logFail(__FILE__, __LINE__, err_msg.str());
    mtx_thr_map.unlock();
    return false;
  } 
  // Getting here means that:
  // (1) the thr_descriptor for ``period'' exists
  // (2) its job queue is empty
  // (3) no closures are attached to this thr_descriptor
  // -> remove from map:
  thr_map.erase(period);
  mtx_thr_map.unlock();
  return true;
}

closure * initFastPeriodicCounterHandler(log_format format) {
  // Set up the closure data structure. Entering this closure into
  // the queue of a sampling thread happens in the below openChannel
  // method. 
  FPC_closure * FPCcl = new FPC_closure();
  FPCcl->handler_f = FastPeriodicCounterHandler;
  FPCcl->parm_handler_f = setArgFastPeriodicCounterHandler;
  FPCcl->ctr = 0;
  FPCcl->total_ctr = 0;
  FPCcl->periodic_inc_ns = FPC_DEFAULT_PERIOD;
  FPCcl->periodic_inc.tv_sec  = FPC_DEFAULT_PERIOD / NANO_S_PER_S;
  FPCcl->periodic_inc.tv_nsec = FPC_DEFAULT_PERIOD % NANO_S_PER_S;
  FPCcl->skipped_intervals = 0;
  FPCcl->isHeaderLogged = false;
  FPCcl->isRefCounted = false;
  return FPCcl;
}

int setArgFastPeriodicCounterHandler(void * cl, int32_t arg_number,
                                     int64_t arg_value) {
  FPC_closure * FPCcl = (FPC_closure *) cl;
  if ((arg_number != 0) && (arg_number != -1)) {
    logFail(__FILE__, __LINE__,
            "setArgFastPeriodicCounterHandler arg number out of range");
    FPCcl->closeChannel(); 
    return -1;
  }
  int64_t value = arg_value;
  if (arg_number == -1) {
    // Setting 'all' arguments at once:
    uint64_t * par = reinterpret_cast<uint64_t *>(arg_value);
    if (par[0] != 1) {
      logFail(__FILE__, __LINE__,
	      "setArgFastPeriodicCounterHandler arg number out of range");
      return -1;
    }
    value = par[0];
  }
  if (value < 0) {
    logFail(__FILE__, __LINE__,
            "setArgFastPeriodicCounterHandler arg 0 val out of range");
      FPCcl->closeChannel(); 
      return -1;
  }
  auto jb = new job(job_type::MOVE_CHANNEL, FPCcl, (uint64_t) value);
  enqueueJobWithThrDescriptor(jb); // async call
  jb->waitCompletion();
  delete jb;
  return 0; 
}

bool FPC_closure::openChannel(int32_t ch_nr, std::string ch_msg, log_format f) {
  // Open the channel's log-file:
  if (!closure::openChannel(ch_nr, ch_msg, f)) {
    // No point to continue without log-file:
    return false;
  }
  // Book-keeping for sampling thread termination check:
  nr_open_channels++;
  this->isRefCounted = true;
  // Create job for the sampling thread:
  auto jb = new job(job_type::INSERT_CHANNEL, this);
  // Enqueue this job with sample thread:
  enqueueJobWithThrDescriptor(jb);
  jb->waitCompletion();
  delete jb;
  return true;
}

void FPC_closure::closeChannel() {
  if (this->isRefCounted) {
    // Book-keeping for sampling thread termination check:
    nr_open_channels--;
    this->isRefCounted = false;
    // Dequeue channel from its sampling thread:
    auto jb = new job(job_type::DELETE_CHANNEL, this);
    // Enqueue DELETE_CHANNEL job with sampling thread:
    dequeueJobFromThrDescriptor(jb);
    jb->waitCompletion();
    delete jb;
  }
  // Unconditionally call closure::closeChannel() to nag if channel
  // is closed multiple times:
  closure::closeChannel();
}

static void printTimeStamp(FPC_closure * FPCcl) {
  writeTimeStampedCtr(FPCcl->ch_logf, FPCcl->sampled_ts,
                      FPCcl->sampled_ctr, true);
}

static void printCtrStart(FPC_closure * FPCcl, uint64_t ctr) {
  struct timespec now;
  clock_gettime_tsc(CLOCK_REALTIME, &now);
  writeTimeStampedCtr(FPCcl->ch_logf, now, ctr, true); 
  char buf_period[1024];
  formatDuration(&FPCcl->periodic_inc, buf_period, 1024, true);
  FPCcl->ch_logf << "# start, period: " << buf_period << std::endl; 
}

static void printTotalCount(FPC_closure * cl) {
  cl->ch_logf << "# total count: " << cl->total_ctr << std::endl;
  cl->ch_logf.flush();
}

static void printTruncatedPeriod(FPC_closure * FPCcl, uint64_t ctr,
                                 std::string msg) {
  struct timespec now;
  clock_gettime_tsc(CLOCK_REALTIME, &now);
  writeTimeStampedCtr(FPCcl->ch_logf, now, ctr, true);
  FPCcl->ch_logf << "# truncated period" << std::endl;
  if (!msg.empty()) {
     FPCcl->ch_logf << "# " << msg << std::endl; 
  }
}

static void printInitialHeaderAndPeriod(FPC_closure * FPCcl) {
  FPCcl->ch_logf << "# <period_end>, <number_of_events>, ";
  writeTimeStampHeader(FPCcl->ch_logf);
  FPCcl->ch_logf << std::endl;
}

static void printSkippedIntervals(FPC_closure * cl) {
  cl->ch_logf << "# skipped ";
  cl->ch_logf << cl->skipped_intervals << " intervals" << std::endl;
}
