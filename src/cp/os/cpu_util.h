#ifndef __CPU_UTIL_H__
#define __CPU_UTIL_H__

#include <atomic>

extern std::atomic<bool> stop_oversubs_chk;

void runOversubsChk();

#endif