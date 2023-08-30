#include "time_tsc.h"

#include "gpiortt_handler.h"
#include "null_handler.h"
#include "log.h"
#include "globals.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <pthread.h>

#define	GPIO_REG_MAP            0xFF634000  // GPIO_REG_MAP
#define FSEL_OFFSET             0x116       // GPIOX_FSEL_REG_OFFSET
#define OUTP_OFFSET             0x117       // GPIOX_OUTP_REG_OFFSET
#define INP_OFFSET              0x118       // GPIOX_INP_REG_OFFSET
#define BLOCK_SIZE              (4*1024)

#define PIN_LINE_M   8  // output on M, input on C
#define PIN_LINE_C   9  // output on C, input on M
#define PIN_BARR_M  10  // output on M, input on C
#define PIN_BARR_C  11  // output on C, input on M
#define LOW(target, pin) (!((target) & (1<<(pin))))
#define PIN(target, pin) ((target>>pin) & 1UL)

#define ITERATIONS (1000 * 1000 * 5)
#define UNROLLING_FACTOR 1
#define CONFIRM_ITER 64

struct GPIORTT_closure;
static bool checkGovernerStatus(GPIORTT_closure * GPIORTTcl);
static bool checkMaxFrequency(GPIORTT_closure * GPIORTTcl);
static bool checkPMUAvailability(GPIORTT_closure * GPIORTTcl);
static bool initGPIO(GPIORTT_closure * GPIORTTcl);

void * GPIORTTHandler_server(void * args);
                        // int tid, GPIORTT_closure* GPIORTTcl);

struct alignas(CACHELINE_SIZE) GPIORTT_closure : closure {
  uint64_t start;
  uint64_t end;
  uint64_t elapsed;
  uint64_t cpu_num;
  uint64_t cpu_khz;

  int fd;
  bool isMaster;
  void *map_base;
  volatile uint32_t *gpio;
  uint32_t confirm_iter;

  uint32_t barr_m;
  uint32_t barr_c;
  uint32_t line_m;
  uint32_t line_c;
};

void GPIORTTHandler_client(void * cl, int64_t tuple) {
  GPIORTT_closure * GPIORTTcl = (GPIORTT_closure *) cl;

  if (GPIORTTcl->isMaster) {
    // Server-side is implemented in GPIORTTHandler_server
  }
  else {
    //------------------------------------------------------------------------
    // GPIORTT (Client)
    
    GPIORTTcl->line_c = !GPIORTTcl->line_c; // sense reverse
    GPIORTTcl->line_m = !GPIORTTcl->line_m; // sense reverse

    // Because GPIORTT_handler is not created unless on aarch64, we just make
    // sure compiliation does not break
#if defined(__aarch64__)
    asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(GPIORTTcl->start));
#endif

    // Signal master (line_c not used; only toggling)
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) ^= (1UL << PIN_LINE_C);
    // Wait for master to signal back
    while (PIN((*(GPIORTTcl->gpio + (INP_OFFSET))),PIN_LINE_M) != GPIORTTcl->line_m) {;}

#if defined(__aarch64__)
    asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(GPIORTTcl->end));
#endif

    GPIORTTcl->elapsed = GPIORTTcl->end-GPIORTTcl->start;

    //
    //------------------------------------------------------------------------
  }

  if (!GPIORTTcl->isMaster) {
  // Invoking clock_gettime is only time-consuming. Because we have tuple
  // number, it's OK to skip getting time in this handler.
  //struct timespec ts;
  //clock_gettime_tsc(CLOCK_REALTIME, &ts);
    GPIORTTcl->ch_logf << tuple << ", " << GPIORTTcl->elapsed << std::endl;
  }
}

closure * initGPIORTTHandler() {
  // Set up the closure data structure.
  GPIORTT_closure * GPIORTTcl = new GPIORTT_closure();
  GPIORTTcl->handler_f = GPIORTTHandler_client;
  GPIORTTcl->parm_handler_f = setArgGPIORTTHandler;
  GPIORTTcl->start    = 0UL;
  GPIORTTcl->end      = 0UL;
  GPIORTTcl->elapsed  = 0UL;
  GPIORTTcl->cpu_num  = 0UL;
  GPIORTTcl->cpu_khz  = 0UL;

  GPIORTTcl->fd           = 0;
  GPIORTTcl->isMaster     = false;
  GPIORTTcl->map_base     = NULL;
  GPIORTTcl->gpio         = NULL;
  GPIORTTcl->confirm_iter = 0U;

  GPIORTTcl->barr_m = 0U;
  GPIORTTcl->barr_c = 0U;
  GPIORTTcl->line_m = 0U;
  GPIORTTcl->line_c = 0U;

  return GPIORTTcl;
}

int setArgGPIORTTHandler(void * cl, int32_t arg_number, int64_t arg_value) {
  GPIORTT_closure * GPIORTTcl = (GPIORTT_closure *) cl;
  if ((arg_number != 2) && (arg_number != 1) && (arg_number != 0) && 
      (arg_number != -1)) {
    logFail(__FILE__, __LINE__,
            "setArgGPIORTTHandler arg number out of range");
    GPIORTTcl->closeChannel(); 
    return -1;
  }
  if (arg_number == -1) {
    // Setting all arguments at once:
    uint64_t * par = reinterpret_cast<uint64_t *>(arg_value);
    if (par[0] != 3) {
      logFail(__FILE__, __LINE__,
              "setArgGPIORTTHandler arg number out of range");
      GPIORTTcl->closeChannel(); 
      return -1;
    }
    int ret = setArgGPIORTTHandler(cl, 0, par[1]);
    if (ret == -1)
      return -1;
    ret = setArgGPIORTTHandler(cl, 1, par[2]);
    if (ret == -1)
      return -1;
    return setArgGPIORTTHandler(cl, 2, par[3]);
  }
  if (arg_number == 0) {
    GPIORTTcl->cpu_num = arg_value;
    if (!checkGovernerStatus(GPIORTTcl)) {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler CPU governer is not configured to performance");
      GPIORTTcl->closeChannel(); 
      return -1;
    }
  }
  if (arg_number == 1) {
    GPIORTTcl->cpu_khz = arg_value;
    if (!checkMaxFrequency(GPIORTTcl)) {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler max/min frequency is not set");
      GPIORTTcl->closeChannel(); 
      return -1;
    }
    if (!checkPMUAvailability(GPIORTTcl)) {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler PMU instruction is not enabled");
      GPIORTTcl->closeChannel(); 
      return -1;
    }
  }
  if (arg_number == 2) {
    if (arg_value == 0) {
      GPIORTTcl->isMaster = false;
    } else if (arg_value == 1) {
      GPIORTTcl->isMaster = true;
    } else {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler Master/Client is not configured");
      GPIORTTcl->closeChannel(); 
      return -1;
    } 
    if (!initGPIO(GPIORTTcl)) {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler Unable to initialize GPIO");
      GPIORTTcl->closeChannel(); 
      return -1;
    }
    if (GPIORTTcl->isMaster) {
      pthread_t thr;
      pthread_attr_t attr;
      pthread_attr_init(&attr);

      if (0 != pthread_create(&thr, &attr, GPIORTTHandler_server,
          static_cast<void *>(GPIORTTcl))) {
        logFail(__FILE__, __LINE__,
            "setArgGPIORTTHandler pthread_create failed");
        GPIORTTcl->closeChannel(); 
        return -1;
      }
      if (0 != pthread_detach(thr)) {
        logFail(__FILE__, __LINE__,
            "setArgGPIORTTHandler pthread_detach failed");
        GPIORTTcl->closeChannel(); 
        return -1;
      }
    } else {
      cpu_set_t mask;
      CPU_ZERO(&mask);

      CPU_SET(GPIORTTcl->cpu_num, &mask);

      if (0 != pthread_setaffinity_np(pthread_self(),
            sizeof(cpu_set_t), &mask)) {
        logFail(__FILE__, __LINE__,
            "setArgGPIORTTHandler pthread_setaffinity_np failed");
        GPIORTTcl->closeChannel(); 
        return -1;
      }

      //------------------------------------------------------------------------
      // BARRIER SYNCHRONIZATION (Client)
      //     On client, barrier synchronization is done here, right after
      //     affinity is set up. However, on master, barrier synchoronization is
      //     done on the server thread.
      //------------------------------------------------------------------------
      GPIORTTcl->barr_c = !GPIORTTcl->barr_c; // sense reverse
      GPIORTTcl->barr_m = !GPIORTTcl->barr_m; // sense reverse
      // Signal master to confirm client is ready
      *(GPIORTTcl->gpio + (OUTP_OFFSET)) ^= (1UL << PIN_BARR_C);
      // Wait for server to get ready
      while (PIN((*(GPIORTTcl->gpio + (INP_OFFSET))),PIN_BARR_M) != GPIORTTcl->barr_m) {;}

    }
  }
  return 0;
}

static bool checkGovernerStatus(GPIORTT_closure * GPIORTTcl) {
  char cmd[256] = {0,};

  sprintf(cmd, "cat /sys/devices/system/cpu/cpu%lu/cpufreq/scaling_governor",
          GPIORTTcl->cpu_num);
  FILE *fd = popen(cmd, "r");
  if (!fd) {
    logFail(__FILE__, __LINE__, "setArgGPIORTTHandler popen failed");
    return false;
  }

  bool checkStatus = true;
  char buf[16] = {0,};
  if (fread (buf, 1, sizeof (buf), fd) > 0) {
    if (strstr(buf, CPU_PERF) == NULL) {
      std::ostringstream errmsg;
      errmsg << "setArgGPIORTTHandler CPU" << GPIORTTcl->cpu_num << ": " << buf;
      logFail(__FILE__, __LINE__, errmsg.str());
      checkStatus = false;
    }
  } else {
    checkStatus = false;
  }

  pclose(fd);
  return checkStatus;
}

static bool checkMaxFrequency(GPIORTT_closure * GPIORTTcl) {
  char cmd[256] = {0,};
  char maxStr[16] = {0,};

  sprintf(maxStr, "%lu", GPIORTTcl->cpu_khz);

  // Check max frequency -------------------------------------------------------
  sprintf(cmd, "cat /sys/devices/system/cpu/cpu%lu/cpufreq/scaling_%s",
          GPIORTTcl->cpu_num, MAX_FREQ);
  FILE *fd = popen(cmd, "r");
  if (!fd) {
    logFail(__FILE__, __LINE__, "setArgGPIORTTHandler popen failed");
    return false;
  }

  bool checkStatus = true;
  char buf[16] = {0,};
  if (fread (buf, 1, sizeof (buf), fd) > 0) {
    if (strstr(buf, maxStr) == NULL) {
      std::ostringstream errmsg;
      errmsg << "setArgGPIORTTHandler CPU" << GPIORTTcl->cpu_num << ": " 
             << MAX_FREQ << ": " << buf << " (" << maxStr << ")";
      logFail(__FILE__, __LINE__, errmsg.str());
      checkStatus = false;
    }
  } else {
    checkStatus = false;
  }

  pclose(fd);

  // Check minimum frequency set to maximum ------------------------------------
  sprintf(cmd, "cat /sys/devices/system/cpu/cpu%lu/cpufreq/scaling_%s",
          GPIORTTcl->cpu_num, MIN_FREQ);
  fd = popen(cmd, "r");
  if (!fd) {
    logFail(__FILE__, __LINE__, "setArgGPIORTTHandler popen failed");
    return false;
  }

  memset(buf, 0x00, sizeof (buf));
  if (fread (buf, 1, sizeof (buf), fd) > 0) {
    if (strstr(buf, maxStr) == NULL) {
      std::ostringstream errmsg;
      errmsg << "setArgGPIORTTHandler CPU" << GPIORTTcl->cpu_num << ": " 
             << MAX_FREQ << ": " << buf << " (" << maxStr << ")";
      logFail(__FILE__, __LINE__, errmsg.str());
      checkStatus = false;
    }
  } else {
    checkStatus = false;
  }

  pclose(fd);

  return checkStatus;
}

static bool checkPMUAvailability(GPIORTT_closure * GPIORTTcl) {
  bool configCheck = true;
  FILE *fd = popen("lsmod | grep " PMU_MODULE, "r");

  char buf[16] = {0,};
  if (fread (buf, 1, sizeof (buf), fd) > 0) {
  } else {
    configCheck = false;
  } 

  pclose(fd);
  return configCheck;
}

static bool initGPIO(GPIORTT_closure * GPIORTTcl) {
  time_t t;
  srand((unsigned) time(&t));
  uint32_t one = (uint32_t)(((double) rand() / (RAND_MAX)) + 1);
  //uint32_t each_iter = 0;

  GPIORTTcl->confirm_iter = one * CONFIRM_ITER;

  if ((GPIORTTcl->fd = open("/dev/gpiomem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
    std::ostringstream errmsg;
    errmsg << "Unable to open /dev/gpiomem. Here is possible solution:\n\n";
    errmsg << "    (1) Double-check for a root-access\n";
    errmsg << "    (2) Add username to a group with an access to the file.\n";
    errmsg << "        See /etc/udev/rules.d/10-odroid.rules\n";
    logFail(__FILE__, __LINE__, errmsg.str());
    return false;
  }

  GPIORTTcl->map_base = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
      GPIORTTcl->fd, GPIO_REG_MAP);
  if (GPIORTTcl->map_base == MAP_FAILED) {
    logFail(__FILE__, __LINE__, "Mmap failed.\n");
    close(GPIORTTcl->fd);
    return false;
  }
  GPIORTTcl->gpio = static_cast<volatile uint32_t *>(GPIORTTcl->map_base);

  // Set all pins input
  *(GPIORTTcl->gpio + (FSEL_OFFSET)) |= 0xFFFFFFFF;

  // Set direction of master pins to output
  if (GPIORTTcl->isMaster) {
    *(GPIORTTcl->gpio + (FSEL_OFFSET)) &= ~(1 << PIN_LINE_M);
    *(GPIORTTcl->gpio + (FSEL_OFFSET)) &= ~(1 << PIN_BARR_M);
  } else {
    *(GPIORTTcl->gpio + (FSEL_OFFSET)) &= ~(1 << PIN_LINE_C);
    *(GPIORTTcl->gpio + (FSEL_OFFSET)) &= ~(1 << PIN_BARR_C);
  }

  // init GPIO lines to 0 (low):
  if (GPIORTTcl->isMaster) {
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_M);  // low
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_M);  // low
  } else {
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_C);  // low
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_C);  // low
  }

  return true;
}

void * GPIORTTHandler_server(void * args) {
  GPIORTT_closure * GPIORTTcl = static_cast<GPIORTT_closure *>(args);
  if (GPIORTTcl->isMaster) {

    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(GPIORTTcl->cpu_num, &mask);

    if (0 != pthread_setaffinity_np(pthread_self(),
          sizeof(cpu_set_t), &mask)) {
      logFail(__FILE__, __LINE__,
          "setArgGPIORTTHandler pthread_setaffinity_np failed");
      GPIORTTcl->closeChannel(); 
      return NULL;
    }

    //------------------------------------------------------------------------
    // BARRIER SYNCHRONIZATION (Master)
    //------------------------------------------------------------------------
    GPIORTTcl->barr_c = !GPIORTTcl->barr_c; // sense reverse
    // Wait for client to get ready
    while (PIN((*(GPIORTTcl->gpio + (INP_OFFSET))),PIN_BARR_C) != GPIORTTcl->barr_c) {;}
    GPIORTTcl->barr_m = !GPIORTTcl->barr_m; // sense reverse
    // Signal client to start experiment (GPIORTTcl->barr_m not used; only toggling)
    *(GPIORTTcl->gpio + (OUTP_OFFSET)) ^= (1UL << PIN_BARR_M);

    //------------------------------------------------------------------------
    // GPIORTT (Master)
    
    while (GPIORTTcl->isMaster) {
      GPIORTTcl->line_c = !GPIORTTcl->line_c; // sense reverse
      GPIORTTcl->line_m = !GPIORTTcl->line_m; // sense reverse

      // Wait for client to signal
      while (PIN((*(GPIORTTcl->gpio + (INP_OFFSET))),PIN_LINE_C) != GPIORTTcl->line_c) {;}
      // Signal back to client (line_m not used; only toggling)
      *(GPIORTTcl->gpio + (OUTP_OFFSET)) ^= (1UL << PIN_LINE_M);
    }

    //
    //------------------------------------------------------------------------

  } else {
    // Client-side is implemented in GPIORTTHandler_client
  }
  return NULL;
}
