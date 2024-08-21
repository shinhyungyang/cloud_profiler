#ifndef __JAVA_AGENT_HANDLER_H__
#define __JAVA_AGENT_HANDLER_H__

#include <string>
#include <stdint.h>

class JavaAgentHandler
{
public:
  virtual ~JavaAgentHandler();
  virtual void callback();
  virtual void null_callback();
  virtual std::string gc_sampler();
  virtual std::string get_jstring();
  virtual const char* get_tuple(int64_t idx);
};

#endif

