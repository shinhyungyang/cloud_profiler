#include "defect_handler.h"
#include "globals.h"
#include "log.h"

#include <sstream>

struct alignas(CACHELINE_SIZE) defect_closure : closure {
  std::string ch_name = "";
  bool handler_f_called = false;
  bool parm_handler_f_called = false;
  virtual bool openChannel(int32_t ch_nr, std::string ch_msg, log_format f);
  virtual void closeChannel(void);
};

bool defect_closure::openChannel(int32_t ch_nr, std::string ch_msg, log_format f) {
  // Defect channels do not require a log file; their status is logged in the
  // cloud_profiler's log file.
  return false;
}

void defect_closure::closeChannel(void) {
  return; // do nothing
}

void DefectHandler(void * cl, int64_t tuple) {
  defect_closure * dcl = (defect_closure *) cl;
  if (!dcl->handler_f_called) {
    dcl->handler_f_called = true;
    // Log this incident:
    std::ostringstream err_msg;
    err_msg << "defect_handler of channel <" << dcl->ch_name << "> ";
    err_msg << "called. Logging of further calls omitted";
    logFail(__FILE__, __LINE__, err_msg.str());
  }
  return;
}

int setArgDefectHandler(void * cl, int32_t arg_number, int64_t arg_value) {
  defect_closure * dcl = (defect_closure *) cl;
  if (!dcl->parm_handler_f_called) {
    dcl->parm_handler_f_called = true;
    // Log this incident:
    std::ostringstream err_msg;
    err_msg << "setArgDefectHandler of channel <" << dcl->ch_name << "> ";
    err_msg << "called. Logging of further calls omitted";
    logFail(__FILE__, __LINE__, err_msg.str());
  }
  return -1;
}

closure * initDefectHandler(std::string ch_name) {
  defect_closure * tmp = new defect_closure();
  tmp->ch_name = ch_name;
  tmp->handler_f = DefectHandler;
  tmp->parm_handler_f = setArgDefectHandler;
  return tmp;
}
