#include "time_tsc.h"

#include "closure.h"
#include "cloud_profiler.h"
#include "globals.h"
#include "JavaAgentHandler.h"

#include <iostream>
#include <thread>
#include <chrono>

#include <sstream>
#include <vector>
#include <iterator>

#include <time.h>

#include <atomic>
#include <cstdlib>

#ifndef NS_PER_S
#define NS_PER_S (1000000000UL)
#endif
#define NUMBER_OF_GC_SAMPLING_ELEMENTS 3

static std::atomic<bool> *stopThread;

JavaAgentHandler::~JavaAgentHandler() {
}

void JavaAgentHandler::callback() {
}

void JavaAgentHandler::null_callback() {
}

std::string JavaAgentHandler::gc_sampler() {
  return (std::string)0;
}

std::string JavaAgentHandler::get_jstring() {
  return (std::string)0;
}

const char* JavaAgentHandler::get_tuple(int64_t idx) {
  return static_cast<const char*>(nullptr);
}

template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

void sampling_thread(JavaAgentHandler* obj, long period, long ch) {
  closure * ch_cl = reinterpret_cast<closure *>(ch);
  struct timespec ts_start, ts_end, ts_diff, ts_duration;
  struct timespec ts_temp;
  long duration_ns;

  ts_temp.tv_sec = period>=1000?period/1000:0;
  ts_temp.tv_nsec = period>=1000?(period-1000)*1000000:period*1000000;

  std::this_thread::sleep_for(std::chrono::milliseconds(period));

  while (*stopThread == false) {
    getTS(&ts_start);
    std::string jstr = obj->gc_sampler();

    std::vector<std::string> _v = split(jstr, ',');
    int _s = _v.size()/NUMBER_OF_GC_SAMPLING_ELEMENTS;
    for (int j=0; j<_s;++j) {
      int _b = j * NUMBER_OF_GC_SAMPLING_ELEMENTS;
      ch_cl->ch_logf << _v[_b+0]
                     << ", " << std::stol(_v[_b+1], nullptr, 10)
                     << ", " << std::stol(_v[_b+2], nullptr, 10) << std::endl;
    }
    getTS(&ts_end);
    subtractTS(&ts_diff, &ts_start, &ts_end);
    if (compareTS(&ts_temp, &ts_diff) > 0) {
      subtractTS(&ts_duration, &ts_diff, &ts_temp);
      duration_ns = (ts_duration.tv_sec*NS_PER_S)+ts_duration.tv_nsec;
      std::this_thread::sleep_for(std::chrono::nanoseconds(duration_ns));
    }
  }
  std::free(stopThread);
  stopThread = static_cast<std::atomic<bool> *>(nullptr);
}

int64_t installAgentHandler(JavaAgentHandler* obj, long period) {
  int64_t ch0 = openChannel("java_agent", ASCII, NULLH);
  stopThread = static_cast<std::atomic<bool> *>(aligned_alloc(CACHELINE_SIZE, sizeof(std::atomic<bool>)));
  *stopThread = false;

  std::thread t1(sampling_thread, obj, period, ch0);
  t1.detach();

  return ch0;
}

void uninstallAgentHandler() {
  if (stopThread != static_cast<std::atomic<bool> *>(nullptr)) {
    *stopThread = true;
  }
}

