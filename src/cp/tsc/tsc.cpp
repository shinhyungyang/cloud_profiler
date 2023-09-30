#include "time_tsc.h"

#include "tsc.h"
#include "log.h"

#if defined (__x86_64__)
#include <cpuid.h>
#endif
#include <iostream>
#include <iomanip>

bool check_rdtsc(void) {
#if defined(__aarch64__)
  return false;
#elif defined(__x86_64__)
  unsigned int eax, ebx, ecx, edx;
  if (!__get_cpuid (1, &eax, &ebx, &ecx, &edx)) {
    logStatus(__FILE__, __LINE__,
              "Warning: CPUID request (value 1) unsupported!");
    return false;
  }
  //std::cout << "EAX: 0x" << std::hex << eax << std::endl;
  //std::cout << "EBX: 0x" << std::hex << ebx << std::endl;
  //std::cout << "ECX: 0x" << std::hex << ecx << std::endl;
  //std::cout << "EDX: 0x" << std::hex << edx << std::endl;
  if (edx & 1 << 4) {
    return true;
  } else {
    return false;
  }
#endif
}

bool check_rdtscp(void) {
#if defined(__aarch64__)
  return false;
#elif defined(__x86_64__)
  unsigned int eax, ebx, ecx, edx;
  if (!__get_cpuid (0x80000001, &eax, &ebx, &ecx, &edx)) {
    logStatus(__FILE__, __LINE__,
              "Warning: CPUID request unsupported!");
    return false;
  }
  //std::cout << "EAX: 0x" << std::hex << eax << std::endl;
  //std::cout << "EBX: 0x" << std::hex << ebx << std::endl;
  //std::cout << "ECX: 0x" << std::hex << ecx << std::endl;
  //std::cout << "EDX: 0x" << std::hex << edx << std::endl;
  if (edx & 1 << 27) {
    return true;
  } else {
    return false;
  }
#endif
}
/*
 * Due to CPU frequency scaling (e.g., Intel's CPU throttling technology,
 * SpeedStep), tsc frequency can be obtained by reading out bogomips from
 * /proc/cpuinfo. Please be noted that BogoMips rating for all Intel and AMD
 * Pentium variations are either exactly 2*clock or approximately 2*clock
 * from kernel version 2.2.14
 *   W. Van Dorst, BogoMips mini-HOWTO, in: The Linux Documentation Project,
 *   ftp://bbc.nvg.org/pub/mirrors/METALAB.UNC.EDU/ldp/HOWTO/pdf/BogoMips.pdf,
 *   2004
 *   https://stackoverflow.com/a/35123505
 *   http://www.clifton.nl/bogo-faq.html
 *
 * Skylake and onwards support CPUID(0x15) instruction which enumerates the
 * TSC/``core crystal clock'' ratio and the nominal frequency of core crystal
 * clock in Hz (ECX may be 0):
 * EAX: the denominator of the TSC/``core crystal clock'' ratio (uint32)
 * EBX: the numerator of the TSC/``core crystal clock'' ratio (uint32)
 * ECX: the nominal frequency of the core crystal clock in Hz
 * 
 * For the case ECX is 0, the nominal frequency of the core crystal clock is not
 * enumerated; pre-defined value ought to be used in such cases.
 * case INTEL_FAM6_SKYLAKE_MOBILE:
 * case INTEL_FAM6_SKYLAKE_DESKTOP:
 * case INTEL_FAM6_KABYLAKE_MOBILE:
 * case INTEL_FAM6_KABYLAKE_DESKTOP:
 *   crystal_khz = 24000;
 *   break;
 * case INTEL_FAM6_ATOM_DENVERTON:
 *   crystal_khz = 25000;
 *   break;
 * case INTEL_FAM6_ATOM_GOLDMONT:
 *   crystal_khz = 19200;
 *   break;
 */
unsigned long get_tsc_freq(void) {
  return 0;
}
