#include "time_tsc.h"

#include "closure.h"
#include "log.h"
#include "globals.h"
#include "handlers/null_handler.h"
#include "handlers/buffer_id_handler.h"
#include "handlers/id_handler.h"
#include "handlers.h"

extern "C" {
#include "net.h"
}

#include <iostream>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstring>

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syscall.h>
}

closure::closure() : handler_f(NULL) {
  //handler_f = NULL;
  parm_handler_f = NULL;
  ch_nr = -1;
  ch_logf_name = NULL;
  isCloseCalled = false;
}

bool closure::openChannel(int32_t ch_nr, std::string ch_msg, log_format format) {
  this->ch_nr = ch_nr;
  // Create filename and open channel's output file:
  char tmp_buf[255];
  if (0 != getIPDottedQuad(tmp_buf, 255)) {
    return false;
  }

  std::ostringstream nvme_output;
  nvme_output << path_to_output << "/md0";
  struct stat st = {0,};
  if (stat(nvme_output.str().c_str(), &st) == -1) {
    mkdir(nvme_output.str().c_str(), 0700);
  }

  pid_t pid = getpid();
  int   tid = syscall(SYS_gettid);
  std::ostringstream name_buf;
  name_buf << nvme_output.str() << "/" << "cloud_profiler:" << tmp_buf; // IP
  name_buf << ":" << pid << ":" << tid << ":ch" << ch_nr << ":";
  // Channel type:
  std::string format_str;
  if (format_to_str(format, &format_str)) {
    name_buf << format_str << ":";
  } else {
    name_buf << "unknown_type:";
  }
  this->format = format;
  // Channel message:
  std::replace (ch_msg.begin(), ch_msg.end(), ' ', '_');
  name_buf << ch_msg << ".txt";
  ch_logf_name = new std::string(name_buf.str());
  std::string msg = "Channel " + *ch_logf_name + std::string(" will be opened");
  logStatus(__FILE__, __LINE__, msg);
  // FIXME: open in binary mode, too.
  if (format == ASCII || format == ASCII_TSC) {
    ch_logf.open(name_buf.str(), std::ios::out | std::ios::trunc);
  } else if (format == BINARY || format == BINARY_TSC || format == ZSTD ||
            format == ZSTD_TSC || format == LZO1X || format == LZO1X) {
    // For now, Binary mode only works for BUFFER ID HANDLER and ID HANDLER
    ch_logf.open(name_buf.str(), std::ios::out |
                                      std::ios::trunc | std::ios::binary);
  } else {
    std::string errmsg = "Unknown log format: " + name_buf.str();
    logFail(__FILE__, __LINE__, errmsg);
    return false;
  }

  if (!ch_logf) {
    std::string errmsg = "open file " + name_buf.str();
    logFail(__FILE__, __LINE__, errmsg);
    return false;
  }
  return true;
}

void closure::closeChannel(void) {
  handler_f = NullHandler;
  if(ch_logf.is_open()) {
    ch_logf.close();
    logStatus(__FILE__, __LINE__, "Channel " + *ch_logf_name + " closed");
  } else {
    if (!isCloseCalled) {
      std::string msg = "Warning: channel " + *ch_logf_name +
                        " attempted to close already closed logfile." +
                        " (Further warnings will be omitted.)";
      logStatus(__FILE__, __LINE__, msg);
      isCloseCalled = true;
    }
  }
}
