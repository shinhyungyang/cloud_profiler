#ifndef __CLOSURE_H__
#define __CLOSURE_H__

// to evade mutual reference with buffer_id_handler.h
struct closure;
struct buffer_id_handler;

#include "time_tsc.h"
#include "cloud_profiler.h"
#include "handlers/buffer_id_handler.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <atomic>

struct closure {
  std::atomic<void (*)(void * closure, int64_t tuple)> handler_f;
  void (*write_handler_f)(std::ofstream * ch_logf, int64_t tuple, timespec * ts);
  int  (*parm_handler_f)(void * closure, int32_t arg_number, int64_t value);
  int32_t       ch_nr;
  std::ofstream ch_logf;
  std::string * ch_logf_name;
  bool isCloseCalled;
  log_format format;
  virtual bool openChannel(int32_t ch_nr, std::string ch_msg, log_format format);
  virtual void closeChannel(void);
  closure();
};

#endif
