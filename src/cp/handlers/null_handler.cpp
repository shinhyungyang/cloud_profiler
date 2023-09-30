#include "null_handler.h"

void NullHandler(void * cl, int64_t tuple) {
  return; // do nothing
}

closure * initNullHandler(log_format format) {
  closure * tmp = new closure();
  tmp->handler_f = NullHandler;
  tmp->parm_handler_f = NULL;
  return tmp;
}

