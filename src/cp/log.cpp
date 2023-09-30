#include "log.h"
#include "globals.h"
#include "cloud_profiler.h"

extern "C" {
#include "net.h"
}

#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
#include <string.h>
#include <assert.h>
}

#define MAX_NAME_LEN 1024
static char outfile_name[MAX_NAME_LEN] = "/tmp/cloud_profiler_logfile";
static bool to_console = CONSOLE_LOG; // Additional log output to console

static std::mutex mtx_outfile;
static std::ofstream logf;

// The status log file should be opened with channel 0, but care 
// must be taken to avoid a race with other channels.

void openStatusLogFile(void) {
  mtx_outfile.lock();
  if (logf.is_open()) {
    // Somebody already opened:
    mtx_outfile.unlock();
    return;
  }
  char tmp_buf[255];
  if (0 != getIPDottedQuad(tmp_buf, 255)) {
    //FIXME: need a strategy what to do if we fail here.
  }
  pid_t pid = getpid();
  std::ostringstream name_buf;  
  name_buf << outfile_name;
  name_buf << ":" << tmp_buf; // IP address
  name_buf << ":" << pid << ".txt";
  logf.open(name_buf.str(), std::ios::out | std::ios::trunc); 
  // FIXME: need strategie what to do if we cannot open this file.
  mtx_outfile.unlock();
}

void closeStatusLogFile(void) {
  mtx_outfile.lock();
  if (logf.is_open()) {
    logf.close();
  }
  mtx_outfile.unlock();
}

void logFail(const char *file, int line, const std::string & msg) {
  mtx_outfile.lock();
  if (!logf.is_open()) {
    mtx_outfile.unlock();
    openStatusLogFile();
    mtx_outfile.lock();
  }
  std::ostringstream msgstrm;
  msgstrm << "ERROR: " << msg << ", line # " << line;
  msgstrm << ", file " << file << std::endl;
  logf << msgstrm.str();
  logf.flush();
  if (to_console) {
    std::cout << msgstrm.str();
    std::cout.flush();
  }
  mtx_outfile.unlock();
}

void logFail(const char *file, int line, const std::string & msg, int err) {
  std::ostringstream msgstrm;
  msgstrm << msg << ", error: " << err << std::endl;
  logFail(file, line, msgstrm.str());
}

void logStatus(const char *file, int line, const std::string & msg) {
  mtx_outfile.lock();
  if (!logf.is_open()) {
    mtx_outfile.unlock();
    openStatusLogFile();
  }
  std::ostringstream msgstrm;
  msgstrm << "STATUS: " << msg << ", file " << file;
  msgstrm << ", line # " << line << std::endl;
  logf << msgstrm.str();
  logf.flush();
  if (to_console) {
    std::cout << msgstrm.str();
    std::cout.flush();
  }
  mtx_outfile.unlock();
}

void logStatus(const char *file, int line, const std::string & msg, int err) {
  std::ostringstream msgstrm;
  msgstrm << msg << ", error: " << err << std::endl;
  logStatus(file, line, msgstrm.str());
}

void toggleConsoleLog(bool flag) {
  to_console = flag;
}

void setLogFileName(const char * name) {
  assert(strlen(name) < MAX_NAME_LEN);
  strncpy(outfile_name, name, MAX_NAME_LEN); 
}
