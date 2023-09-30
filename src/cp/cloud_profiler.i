
%module(directors="1") cloud_profiler
%{
#include "cloud_profiler.h"
#include "java_agent/JavaAgentHandler.h"
#include <iostream>
#include <sstream>
#include <zmq.hpp>
%}

%include "stdint.i"      // C++11 fixed-width int types
%include "std_string.i"  // std::string -> char * typemaps
//%include "std_map.i"
//%include "carrays.i"
%include "various.i"     // BYTE typemap
//%include "std_list.i"
%include "std_vector.i"
%include "swig/byte2d.i"

%apply (char *BYTE) { (char *s) }; // Replace *s by BYTE typemap.
%apply char **BYTE2D { (char ** byte2D) }

// Force the generated Java code to use the C enum values rather than
// making a JNI call:
%javaconst(1) log_format;
%javaconst(1) handler_type;

// Java constant definitions:
%javaconst(1) MILLI_S_PER_S;
%constant int64_t MILLI_S_PER_S = 1000;

%javaconst(1) MICRO_S_PER_S;
%constant int64_t MICRO_S_PER_S = 1000000;

%javaconst(1) NANO_S_PER_S;
%constant int64_t NANO_S_PER_S  = 1000000000;

%javaconst(1) NANO_S_PER_MIN;
%constant int64_t NANO_S_PER_MIN = 60 * NANO_S_PER_S;

%javaconst(1) NANO_S_PER_HOUR;
%constant int64_t NANO_S_PER_HOUR = 3600 * NANO_S_PER_S;

%javaconst(1) NANO_S_PER_DAY;
%constant int64_t NANO_S_PER_DAY = 24 * 3600 * NANO_S_PER_S;

// Dependency on system header, which is hard to parse with SWIG.
// See clock_gettime manpage and "typetest" program for details.
struct timespec {
  int64_t tv_sec;
  int64_t tv_nsec;
};

%begin %{
#include "time_tsc.h"
%}

%header %{
#define CACHELINE_SIZE  64

struct alignas(CACHELINE_SIZE) timeout_vars {
  uint64_t next_timeout  __attribute__((aligned(CACHELINE_SIZE)));
};

uint64_t elapsedIntervals(int64_t ch);
timeout_vars* getTimeoutVars(int64_t ch, int tid);
void* fetchZMQStrNB2(int64_t ch, zmq::message_t* msg);
%}


%typemap(newfree) (std::string str1) {
  delete $1;
}
%typemap(in) (std::string str1) () {
  jlong jresult = 0;
  const char * $1_pstr = (const char *)jenv->GetStringUTFChars($input, 0);
  if (!$1_pstr) return $null;
  if ($1_pstr) {
    result = new std::string($1_pstr);
    jenv->ReleaseStringUTFChars($input, $1_pstr);
    *(std::string **)&jresult = result;
    return jresult;
  }
}
%newobject newString;

%typemap(in) (std::string str2) () {
  const char * s = (const char *)jenv->GetStringUTFChars($input, 0);
  if (!s) return $null;
  $1.assign(s);
  jenv->ReleaseStringUTFChars($input, s);
}

%typemap(out) const char* {
  if ($1) $result = JCALL1(NewStringUTF, jenv, (const char *)$1);
  delete $1;
}

// A hack to avoid deleting memory allocation
%typemap(out) char* {
  $result = JCALL1(NewStringUTF, jenv, (const char *)$1);
}

// Swig's STRING_ARRAY implementation is inadequate for our use. Therefore,
// define our own type BUCKET
%typemap(jni) char **BUCKET "jobjectArray"
%typemap(jtype) char **BUCKET "String[]"
%typemap(jstype) char **BUCKET "String[]"
%typemap(out) char**BUCKET {
  $result = bucket2;
}
%typemap(javain) char **BUCKET "$javainput"
%typemap(javaout) char **BUCKET {
    return $jnicall;
}

// Explicitly apply BUCKET type to prevent all char ** mapped into String[]
%apply char **BUCKET { (char ** ZMQBucket) }
%apply char **BUCKET { (char ** ZBktOpt) }
%apply char **BUCKET { (char ** ZBktOpt2) }
%apply char **BUCKET { (char ** ZBktOpt3) }
%apply char **BUCKET { (char ** CStrBucket) }

%typemap(in) (char** ZMQBucket) {
  uint64_t start_ns, elapsed_ns;
  char* zmqStr;
  const int TIMEOUT_NS = 10000000;
  int i = 0;
  start_ns = elapsedIntervals(arg1);
  do {
    do {
      elapsed_ns = elapsedIntervals(arg1);
      if ((elapsed_ns - start_ns) >= TIMEOUT_NS) {
        goto bucket_done;
      }
      zmqStr = fetchZMQStrNB(arg1);
      // memorize the time stamp of the last successful receive
      if (nullptr != zmqStr) {
        start_ns = elapsed_ns;
      }
    } while (nullptr == zmqStr) ;
    // add string to bucket:
    jstring jstr = JCALL1(NewStringUTF, jenv, zmqStr);
    JCALL3(SetObjectArrayElement, jenv, $input, i, jstr);
    JCALL1(DeleteLocalRef, jenv, jstr);
  } while (++i < arg2) ;
bucket_done:
  return i;
}

%typemap(in) (char** ZBktOpt) {
  char* zmqStr;
  jstring jstr;
  int i = 0;
  timeout_vars* tmo = getTimeoutVars(arg1, arg3);
  while (true) {
    tmo->next_timeout = elapsedIntervals(arg1) + 1;
    do {
      do {
        if (elapsedIntervals(arg1) >= tmo->next_timeout) {
          goto bucket_done;
        }
        zmq::message_t zmqmsg;
        zmqStr = static_cast<char*>(fetchZMQStrNB2(arg1, &zmqmsg));
        if (nullptr == zmqStr) {
          continue;
        }
        else {
          jstr = JCALL1(NewStringUTF, jenv, zmqStr);
          break;
        }
      } while (true) ;
      // add string to bucket:
      JCALL3(SetObjectArrayElement, jenv, $input, i, jstr);
      JCALL1(DeleteLocalRef, jenv, jstr);
    } while (++i < arg2) ;
bucket_done:
    if (i > 0) {
      break;
    }
  }
  return i;
}

%typemap(in) (char** ZBktOpt2) {
  char* zmqStr;
  jstring jstr;
  int i = 0;
  timeout_vars* tmo = getTimeoutVars(arg1, arg3);
  tmo->next_timeout = elapsedIntervals(arg1) + 1;
  do {
    do {
      if (elapsedIntervals(arg1) >= tmo->next_timeout) {
        goto bucket_done;
      }
      zmq::message_t zmqmsg;
      zmqStr = static_cast<char*>(fetchZMQStrNB2(arg1, &zmqmsg));
      if (nullptr == zmqStr) {
        continue;
      }
      else {
        jstr = JCALL1(NewStringUTF, jenv, zmqStr);
        break;
      }
    } while (true) ;
    // add string to bucket:
    JCALL3(SetObjectArrayElement, jenv, $input, i, jstr);
    JCALL1(DeleteLocalRef, jenv, jstr);
  } while (++i < arg2) ;
bucket_done:
  return i;
}

%typemap(in) (char** ZBktOpt3) {
  uint64_t timeout_ns;
  char* zmqStr;
  jstring jstr;
  const int TIMEOUT_NS = 1000000; // 1ms
  int i = 0;
  timeout_vars* tmo = getTimeoutVars(arg1, arg3);
  timeout_ns = tmo->next_timeout;
  do {
    do {
      if (elapsedIntervals(arg1) >= timeout_ns) {
        goto bucket_done;
      }
      zmq::message_t zmqmsg;
      zmqStr = static_cast<char*>(fetchZMQStrNB2(arg1, &zmqmsg));
      if (nullptr == zmqStr) {
        continue;
      }
      else {
        jstr = JCALL1(NewStringUTF, jenv, zmqStr);
        break;
      }
    } while (true) ;
    // add string to bucket:
    JCALL3(SetObjectArrayElement, jenv, $input, i, jstr);
    JCALL1(DeleteLocalRef, jenv, jstr);
  } while (++i < arg2) ;
bucket_done:
  tmo->next_timeout += TIMEOUT_NS; // increases in lockstep
  return i;
}

%typemap(in) (char** CStrBucket) {
  int i = 0;
  if ($input) {
    jint size = JCALL1(GetArrayLength, jenv, $input);
    for (i = 0; i<size; i++) {
      char* str = fetchCStr();
      jstring jstr = JCALL1(NewStringUTF, jenv, str);
      JCALL3(SetObjectArrayElement, jenv, $input, i, jstr);
      JCALL1(DeleteLocalRef, jenv, jstr);
    }
  }
}

%include "cloud_profiler.h"

// These return std::string which the caller must free:
//%newobject formatTS1;

// Generate a director for JavaAgentHandler
%feature("director") JavaAgentHandler;
%include "java_agent/JavaAgentHandler.h"

typedef std::string * StrPtr;
%template(LongVec) std::vector<long>;
%template(StringVec) std::vector<std::string>;
%template(StrPtrVec) std::vector<StrPtr>;

