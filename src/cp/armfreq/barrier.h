#ifndef __BARRIER_H__
#define __BARRIER_H__ // prevent against double-inclusion

// Shared-memory barrier implementation until std::experimental::barrier
// is supported by compilers.

// This implementation counts the number of barrier_wait() calls of each thread
// in a separate 64-bit counter. Those counters will overflow after 146 years
// if the thread-ensemble synchronizes at the barrier at a frequency of 4 GHz
// (4*10^9 barrier-synchronizations per second).

extern "C"{
  void barrier_init(int nr_threads);
  // Initialize barrier to specified number of nr_threads

  void barrier_wait(int tid);
  // Busy-wait until all participating threads arrived at the barrier.
}

#endif

