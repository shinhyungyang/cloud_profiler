#include "time_tsc.h"

#include "cloud_profiler.h"
#include "log.h"
#include "config_server/net_conf.h"
#include "globals.h"
#include "closure.h"
#include "os/cpu_util.h"
#include "handlers/null_handler.h"
#include "handlers/id_handler.h"
#include "handlers/buffer_id_handler.h"
#include "handlers/io_thread/cleanup.h"
#include "handlers/downsample_handler.h"
#include "handlers/XoY_handler.h"
#include "handlers/firstlast_handler.h"
#include "handlers/median_handler.h"
#include "handlers/periodic_counter_handler.h"
#include "handlers/fast_periodic_counter_handler.h"
#include "handlers/defect_handler.h"
#include "handlers/fox_start_handler.h"
#include "handlers/fox_end_handler.h"
#include "handlers/gpiortt_handler.h"

extern "C" {
#include "util.h"
#include "net.h"
}

#include <cstdint>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <mutex>
#include <thread>

extern "C" {
#include <sys/types.h> 
#include <unistd.h> 
#include <syscall.h> 
}

static std::atomic<int32_t> next_channel(0);

std::vector<struct closure *> ch_info;

std::mutex mtx_bk;

alignas(CACHELINE_SIZE) std::atomic<bool> oversubs_chk(false);
std::thread * oversubs_chk_th;

int64_t openChannel(std::string ch_msg, log_format f, handler_type h) {
  if (next_channel == 0) {
    // Open the logger's status file before we run out of file handles:
    openStatusLogFile();
  }
  int32_t mychannel = next_channel++;
  closure * new_ch;
  handler_type ch_h = h;
  int64_t par[conf_server_max_param + 1];
  par[0] = 0; //default parameter number 0
  if (h == NET_CONF) {

    // Retrieve channel configuration from configuration server:
    bool ret = getConfigFromServer(ch_msg, mychannel, &ch_h, par, &f);
    if (!ret) {
      std::ostringstream err_msg;
      err_msg << "Ch: " << ch_msg;
      err_msg << ", ch_nr " << mychannel;
      err_msg << ": unable to obtain configuration from server";
      logFail(__FILE__, __LINE__, err_msg.str());
      return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
    }
  }

  if (!oversubs_chk.exchange(true))
    oversubs_chk_th = new std::thread(runOversubsChk);

  switch (ch_h) {
    case IDENTITY:
      new_ch = initIdentityHandler(f);
      break;
    case BUFFER_IDENTITY:
      new_ch = initBufferIdentityHandler(f, false);
      break;
    case BUFFER_IDENTITY_COMP:
      new_ch = initBufferIdentityHandler(f, true);
      break;
    case MEDIAN:
      new_ch = initMedianHandler(f);
      break;
    case DOWNSAMPLE:
      new_ch = initDownsampleHandler(f);
      break;
    case XOY:
      new_ch = initXoYHandler(f);
      break;
    case FIRSTLAST:
      new_ch = initFirstlastHandler(f);
      break;
    case PERIODICCOUNTER:
      new_ch = initPeriodicCounterHandler(f);
      break;
    case FASTPERIODICCOUNTER:
      new_ch = initFastPeriodicCounterHandler(f);
      break;
    case NULLH:
      new_ch = initNullHandler(f);
      break;
    case FOX_START:
      new_ch = initFoxStartHandler(ch_msg, mychannel, &ch_h, par, &f);
      break;
    case FOX_END:
      new_ch = initFoxEndHandler(ch_msg, mychannel, &ch_h, par, &f);
      break;
    case GPIORTT:
#if defined (__aarch64__)
      new_ch = initGPIORTTHandler(f);
      break;
#else
      new_ch = initNullHandler(f);
      {
        std::ostringstream err_msg2;
        err_msg2 << "Ch: \"" << ch_msg << "\" supported only on aarch64";
        logFail(__FILE__, __LINE__, err_msg2.str());
      }
      return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
      break;
#endif
    default:
      new_ch = initNullHandler(f);
      std::ostringstream err_msg;
      err_msg << "Ch: \"" << ch_msg << "\" unsupported handler type";
      logFail(__FILE__, __LINE__, err_msg.str());
      return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
      break;
  }
  mtx_bk.lock();
  // Set cleanup callback function
  if (ch_info.size() == 0) {
    // FIXME: if the signal function is registered already,
    // it also should be called
    std::atexit(cleanUpAtNormalExit);
    registerAbnormalExitHandler();
  }
  ch_info.push_back(new_ch);
  mtx_bk.unlock();
  new_ch->ch_nr = mychannel;
  // Open channel's log file:
  if (!new_ch->openChannel(mychannel, ch_msg, f)) {
    return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
  }
  if (h == NET_CONF) {
    // Parameterize the channel with the arguments from the
    // configuration server:
    if (par[0] > 0) {
      // We received arguments, funnel them through to the handler:
      if (new_ch->parm_handler_f == NULL) {
        std::ostringstream err_msg;
        err_msg << "Ch: " << ch_msg;
        err_msg << ", ch_nr " << mychannel;
        err_msg << ": closure without handler function";
	      err_msg << ", check channel's network configuration";
        logFail(__FILE__, __LINE__, err_msg.str());
        return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
      }
      int64_t ptr_as_uint = reinterpret_cast<int64_t>(par);
      int retv = new_ch->parm_handler_f(new_ch, -1, ptr_as_uint);
      if (retv == -1) {
        std::ostringstream err_msg;
        err_msg << "Ch: " << ch_msg;
        err_msg << ", ch_nr " << mychannel;
        err_msg << ": unable to set configuration arguments";
        logFail(__FILE__, __LINE__, err_msg.str());
        return reinterpret_cast<intptr_t>(initDefectHandler(ch_msg));
      }
    }
  }
  return reinterpret_cast<intptr_t>(new_ch);
}

bool isOpen(int64_t ch) {
  // Conditions for a channel to be open:
  // 1) channel member of ch_info (this excludes defect channels)
  // 2) channel log file's is_open() method returns true
  closure * ch_cl = reinterpret_cast<closure *>(ch);
  std::lock_guard<std::mutex> guard(mtx_bk); // RAII
  for (auto const & cl: ch_info) {
    if (cl == ch_cl) {
      return cl->ch_logf.is_open();
    }
  }
  return false;
}

void closeChannel(int64_t ch) {
  closure * ch_cl = reinterpret_cast<closure *>(ch);
  ch_cl->closeChannel();
}

int parameterizeChannel(int64_t ch, int32_t arg_number, int64_t arg_value) {
  closure * ch_cl = reinterpret_cast<closure *>(ch);
  if (ch_cl->parm_handler_f == NULL) {
    // User attempts to parameterize channel which does not support it.
    // Close channel and log error:
    ch_cl->closeChannel();
    std::ostringstream error_strm;
    error_strm << "Ch: " << ch_cl->ch_nr << " does not support parameterization.";
    std::string error_str = error_strm.str();
    logFail(__FILE__, __LINE__, error_str);
    return -1;
  }
  return ch_cl->parm_handler_f(ch_cl, arg_number, arg_value);
}

int parameterizeChannel(int64_t ch, 
                        int32_t arg_number, 
                        const std::string arg_value)
{
  // Convert the arg_value string to int64_t and defer the rest to the
  // int64_t-typed function:
  struct conv_result r;
  convertA2Inoexit(arg_value.c_str(), &r);
  if (1 != r.valid) {
    // Invalid literal value:
    closure * ch_cl = reinterpret_cast<closure *>(ch);
    ch_cl->closeChannel();
    std::ostringstream error_strm;
    error_strm << "Ch: " << ch_cl->ch_nr << " invalid parameter value: ";
    error_strm << r.err_msg;
    std::string error_str = error_strm.str();
    logFail(__FILE__, __LINE__, error_str);
  }
  return parameterizeChannel(ch, arg_number, (int64_t) r.result);
}

void logTS(int64_t ch, int64_t tuple) {
  closure * ch_cl = reinterpret_cast<closure *>(ch);
  //std::ostringstream tmp;
  //tmp << "Ch: " << ch_cl->ch_nr << " Tuple: " << tuple;
  //std::string foo = tmp.str();
  //logStatus(__FILE__, __LINE__, foo);
  if (tuple == -1) {
    // Poison pill:
    ch_cl->closeChannel();
    return;
  }
  ch_cl->handler_f(ch_cl, tuple);
}

void nologTS(int64_t ch, int64_t tuple) {
  return;
}
