#include "time_tsc.h"

#include "time_io.h"
#include "globals.h"
#include "cloud_profiler.h"

#include <cstdint>

void writeTimeStampedCtr(std::ostream & outf, struct timespec & ts,
                    uint64_t ctr, bool newline) {
  char buf[1024];
  formatTS(&ts, buf, 1024);
  outf << buf << ", " << ctr;
  outf << ", [" << ts.tv_sec << ", " << ts.tv_nsec << ", " << ts.tsc << "]";
  if (newline) {
    outf << std::endl;
  }
}

void writeTimeStampHeader(std::ostream & outf) {
  outf << "[period_end <s>, <ns>, <tsc>]";
}
