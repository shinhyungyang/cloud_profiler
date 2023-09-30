#include<atomic>
#include<cstdint>

#include"globals.h"

struct alignas(CACHELINE_SIZE) aligned_ctr {
  std::atomic<uint64_t> ctr;
};

aligned_ctr ctr[MAX_ALLOC_NR_THREADS];

static int nr_threads;

extern "C"{
  void barrier_init(int nr_threads) {
    ::nr_threads = nr_threads;
    for (int i = 0; i<nr_threads; i++) {
      ctr[i].ctr = 0;
    }
  }
  
  void barrier_wait(int tid) {
    ctr[tid].ctr += 1;
    uint64_t my_level = ctr[tid].ctr;
    for (int thr = 0; thr < nr_threads; thr++) {
      while (ctr[thr].ctr < my_level) { ; }
    }
  }
} 
