#include "time_tsc.h"
#include "cloud_profiler.h"
#include "id_handler.h"
#include "log.h"
#include <iostream>
#include <fstream>

struct alignas(CACHELINE_SIZE) id_closure : closure {
  bool first_logging;
};

void IdentityHandler(void * cl, int64_t tuple) {
  closure * ch_cl = static_cast<closure *>(cl);
  struct timespec ts;
  clock_gettime_tsc(CLOCK_REALTIME, &ts);

  ch_cl->write_handler_f(&(ch_cl->ch_logf), tuple, &ts);
}

void IdentityHandlerTsc(void * cl, int64_t tuple) {
  id_closure * ch_cl = static_cast<id_closure *>(cl);
  struct timespec ts;

  if (ch_cl->first_logging) {
    ch_cl->first_logging = false;
    clock_gettime_tsc(CLOCK_REALTIME, &ts);
    ch_cl->write_handler_f(&(ch_cl->ch_logf), tuple, &ts);
    ch_cl->write_handler_f = (ch_cl->write_handler_f == writeInAscii ?
                                          writeInAsciiTsc : writeInBinaryTsc);
  } else {
    clock_gettime_tsc_only(CLOCK_REALTIME, &ts);
    ch_cl->write_handler_f(&(ch_cl->ch_logf), tuple, &ts);
  }
}

closure * initIdentityHandler(log_format format) {
  id_closure * tmp = new id_closure();
  tmp->first_logging = true;
  tmp->parm_handler_f = NULL;
  switch (format) {
  case ASCII:
    tmp->handler_f = IdentityHandler;
    tmp->write_handler_f = writeInAscii;
    break;
  case ASCII_TSC:
    tmp->handler_f = IdentityHandlerTsc;
    tmp->write_handler_f = writeInAscii;
    break;
  case BINARY:
    tmp->handler_f = IdentityHandler;
    tmp->write_handler_f = writeInBinary;
    break;
  case BINARY_TSC:
    tmp->handler_f = IdentityHandlerTsc;
    tmp->write_handler_f = writeInBinary;
  }
  return tmp;
}

void writeInAscii(std::ofstream * ch_logf, int64_t tuple, timespec * ts) {
  *ch_logf << tuple << ", " << ts->tv_sec << ", " << ts->tv_nsec
                            << ", " << ts->tsc << ", " << ts->aux << '\n';
}

void writeInAsciiTsc(std::ofstream * ch_logf, int64_t tuple, timespec * ts) {
  *ch_logf << tuple << ", " << ts->tsc << ", " << ts->aux << '\n';
}

void writeInBinary(std::ofstream * ch_logf, int64_t tuple, timespec * ts) {
  ch_logf->write((char *)&tuple, sizeof(tuple));
  ch_logf->write((char *)ts, sizeof(*ts));
}

void writeInBinaryTsc(std::ofstream * ch_logf, int64_t tuple, timespec * ts) {
  ch_logf->write((char *)&tuple, sizeof(tuple));
  ch_logf->write((char *)&(ts->tsc), sizeof(ts->tsc));
}

void readInBinary(std::ifstream * log_to_read, int64_t * tuple, timespec * ts) {
  log_to_read->read((char *)tuple, sizeof(tuple));
  log_to_read->read((char *)ts, sizeof(*ts));
}
