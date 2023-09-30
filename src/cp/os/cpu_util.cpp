// https://gitlab.com/procps-ng/procps

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <atomic>

extern "C" {
#include <unistd.h>
#include <sys/select.h>
}

#include "globals.h"
#include "log.h"

#define SMLBUFSIZ   128
#define MEDBUFSIZ   256
#define BIGBUFSIZ  2048

      /* These typedefs attempt to ensure consistent 'ticks' handling */
typedef unsigned long long TIC_t;
typedef          long long SIC_t;

        /* These 2 structures store a frame's cpu tics used in history
           calculations.  They exist primarily for SMP support but serve
           all environments. */
typedef struct CT_t {
  /* other kernels: u == user/us, n == nice/ni, s == system/sy, i == idle/id
      2.5.41 kernel: w == IO-wait/wa (io wait time)
      2.6.0  kernel: x == hi (hardware irq time), y == si (software irq time)
      2.6.11 kernel: z == st (virtual steal time) */
  TIC_t u, n, s, i, w, x, y, z;  // as represented in /proc/stat
#ifndef CPU_ZEROTICS
  SIC_t tot;                     // total from /proc/stat line 1
#endif
} CT_t;

typedef struct CPU_t {
  CT_t cur;                      // current frame's cpu tics
  CT_t sav;                      // prior frame's cpu tics
#ifndef CPU_ZEROTICS
  SIC_t edge;                    // tics adjustment threshold boundary
#endif
  int id;                        // cpu number (0 - nn), or numa active flag
  int node;                      // the numa node it belongs to
} CPU_t;

#ifndef CPU_ZEROTICS
        /* This is the % used in establishing the tics threshold below
           which a cpu is treated as 'idle' rather than displaying
           misleading state percentages */
#define TICS_EDGE  20
#endif

#define MALLOC __attribute__ ((__malloc__))

static CPU_t      *Cpu_tics;
static int         Cpu_faux_tot;
static int Numa_node_tot;

long smp_num_cpus;     /* number of CPUs */

void cpuinfo (void) {
  // ought to count CPUs in /proc/stat instead of relying
  // on glibc, which foolishly tries to parse /proc/cpuinfo
  // note: that may have been the case but now /proc/stat
  //       is the default source.  parsing of /proc/cpuinfo
  //       only occurs if the open on /proc/stat fails
  //
  // SourceForge has an old Alpha running Linux 2.2.20 that
  // appears to have a non-SMP kernel on a 2-way SMP box.
  // _SC_NPROCESSORS_CONF returns 2, resulting in HZ=512
  // _SC_NPROCESSORS_ONLN returns 1, which should work OK

  smp_num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (smp_num_cpus<1)        /* SPARC glibc is buggy */
  smp_num_cpus=1;
}

/*######  Low Level Memory/Keyboard/File I/O support  ####################*/

        /*
         * Handle our own memory stuff without the risk of leaving the
         * user's terminal in an ugly state should things go sour. */

static void *alloc_c (size_t num) MALLOC;
static void *alloc_c (size_t num) {
  void *pv;

  if (!num) ++num;
  if (!(pv = calloc(1, num)))
    printf("Fail to allocate memory!\n");
  return pv;
} // end: alloc_c

static void *alloc_r (void *ptr, size_t num) MALLOC;
static void *alloc_r (void *ptr, size_t num) {
  void *pv;

  if (!num) ++num;
  if (!(pv = realloc(ptr, num)))
    printf("Fail to allocate memory!\n");
  return pv;
} // end: alloc_r

        /*
         * This guy's modeled on libproc's 'eight_cpu_numbers' function except
         * we preserve all cpu data in our CPU_t array which is organized
         * as follows:
         *    Cpu_tics[0] thru Cpu_tics[n] == tics for each separate cpu
         *    Cpu_tics[sumSLOT]            == tics from /proc/stat line #1
         *  [ and beyond sumSLOT           == tics for each cpu NUMA node ] */
static void cpus_refresh (void) {
#define sumSLOT ( smp_num_cpus )
#define totSLOT ( 1 + smp_num_cpus + 1)
  static FILE *fp = NULL;
  static int siz;//, sav_slot = -1;
  static char *buf;
  CPU_t *sum_ptr;                               // avoid gcc subscript bloat
  int i, num, tot_read;
  int node;
  char *bp;

   /* by opening this file once, we'll avoid the hit on minor page faults
      (sorry Linux, but you'll have to close it for us) */
  if (!fp) {
    if (!(fp = fopen("/proc/stat", "r")))
      printf("Fail to open /proc/stat!\n");
      /* note: we allocate one more CPU_t via totSLOT than 'cpus' so that a
               slot can hold tics representing the /proc/stat cpu summary */
    Cpu_tics = (CPU_t *) alloc_c(totSLOT * sizeof(CPU_t));
  }
  rewind(fp);
  fflush(fp);

 #define buffGRW 8192
   /* we slurp in the entire directory thus avoiding repeated calls to fgets,
      especially in a massively parallel environment.  additionally, each cpu
      line is then frozen in time rather than changing until we get around to
      accessing it.  this helps to minimize (not eliminate) most distortions. */
  tot_read = 0;
  if (buf) buf[0] = '\0';
  else buf = (char *) alloc_c((siz = buffGRW));
  while (0 < (num = fread(buf + tot_read, 1, (siz - tot_read), fp))) {
    tot_read += num;
    if (tot_read < siz) break;
    buf = (char *) alloc_r(buf, (siz += buffGRW));
  };
  buf[tot_read] = '\0';
  bp = buf;
 #undef buffGRW

  // remember from last time around
  sum_ptr = &Cpu_tics[sumSLOT];
  memcpy(&sum_ptr->sav, &sum_ptr->cur, sizeof(CT_t));
  // then value the last slot with the cpu summary line
  if (4 > sscanf(bp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu"
      , &sum_ptr->cur.u, &sum_ptr->cur.n, &sum_ptr->cur.s
      , &sum_ptr->cur.i, &sum_ptr->cur.w, &sum_ptr->cur.x
      , &sum_ptr->cur.y, &sum_ptr->cur.z))
         printf("Fail to sscanf cpu!\n");
#ifndef CPU_ZEROTICS
  sum_ptr->cur.tot = sum_ptr->cur.u + sum_ptr->cur.s
      + sum_ptr->cur.n + sum_ptr->cur.i + sum_ptr->cur.w
      + sum_ptr->cur.x + sum_ptr->cur.y + sum_ptr->cur.z;
   /* if a cpu has registered substantially fewer tics than those expected,
      we'll force it to be treated as 'idle' so as not to present misleading
      percentages. */
  sum_ptr->edge =
      ((sum_ptr->cur.tot - sum_ptr->sav.tot) / smp_num_cpus) / (100 / TICS_EDGE);
#endif

  // forget all of the prior node statistics (maybe)
  memset(sum_ptr + 1, 0, Numa_node_tot * sizeof(CPU_t));

  // now value each separate cpu's tics...
  for (i = 0; i < sumSLOT; i++) {
    CPU_t *cpu_ptr = &Cpu_tics[i];           // avoid gcc subscript bloat
#ifdef PRETEND48CPU
    int j;
    if (i == 0) j = 0; else ++j;
    if (j >= Cpu_true_tot) { bp = buf; j = 0; }
#endif
    bp = 1 + strchr(bp, '\n');
    // remember from last time around
    memcpy(&cpu_ptr->sav, &cpu_ptr->cur, sizeof(CT_t));
    if (4 > sscanf(bp, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu", &cpu_ptr->id
         , &cpu_ptr->cur.u, &cpu_ptr->cur.n, &cpu_ptr->cur.s
         , &cpu_ptr->cur.i, &cpu_ptr->cur.w, &cpu_ptr->cur.x
         , &cpu_ptr->cur.y, &cpu_ptr->cur.z)) {
            break;        // tolerate cpus taken offline
    }

#ifndef CPU_ZEROTICS
    cpu_ptr->edge = sum_ptr->edge;
#endif
#ifdef PRETEND48CPU
    cpu_ptr->id = i;
#endif
      /* henceforth, with just a little more arithmetic we can avoid
         maintaining *any* node stats unless they're actually needed */
    if (Numa_node_tot
      && -1 < (node = -1)) {
         // use our own pointer to avoid gcc subscript bloat
      CPU_t *nod_ptr = sum_ptr + 1 + node;
      nod_ptr->cur.u += cpu_ptr->cur.u; nod_ptr->sav.u += cpu_ptr->sav.u;
      nod_ptr->cur.n += cpu_ptr->cur.n; nod_ptr->sav.n += cpu_ptr->sav.n;
      nod_ptr->cur.s += cpu_ptr->cur.s; nod_ptr->sav.s += cpu_ptr->sav.s;
      nod_ptr->cur.i += cpu_ptr->cur.i; nod_ptr->sav.i += cpu_ptr->sav.i;
      nod_ptr->cur.w += cpu_ptr->cur.w; nod_ptr->sav.w += cpu_ptr->sav.w;
      nod_ptr->cur.x += cpu_ptr->cur.x; nod_ptr->sav.x += cpu_ptr->sav.x;
      nod_ptr->cur.y += cpu_ptr->cur.y; nod_ptr->sav.y += cpu_ptr->sav.y;
      nod_ptr->cur.z += cpu_ptr->cur.z; nod_ptr->sav.z += cpu_ptr->sav.z;
#ifndef CPU_ZEROTICS
         /* yep, we re-value this repeatedly for each cpu encountered, but we
            can then avoid a prior loop to selectively initialize each node */
      nod_ptr->edge = sum_ptr->edge;
#endif
      cpu_ptr->node = node;
#ifndef OFF_NUMASKIP
      nod_ptr->id = -1;
#endif
    }
  } // end: for each cpu

  Cpu_faux_tot = i;      // tolerate cpus taken offline
 #undef sumSLOT
 #undef totSLOT
} // end: cpus_refresh

        /*
         * State display *Helper* function to calc and display the state
         * percentages for a single cpu.  In this way, we can support
         * the following environments without the usual code bloat.
         *    1) single cpu machines
         *    2) modest smp boxes with room for each cpu's percentages
         *    3) massive smp guys leaving little or no room for process
         *       display and thus requiring the cpu summary toggle */
static float cpu_tics (CPU_t *cpu, const char *pfx, int nobuf) {
   /* we'll trim to zero if we get negative time ticks,
      which has happened with some SMP kernels (pre-2.4?)
      and when cpus are dynamically added or removed */
 #define TRIMz(x)  ((tz = (SIC_t)(x)) < 0 ? 0 : tz)
   //    user    syst    nice    idle    wait    hirg    sirq    steal
  SIC_t u_frme, s_frme, n_frme, i_frme, w_frme, x_frme, y_frme, z_frme, tot_frme, tz;
  float scale;

  u_frme = TRIMz(cpu->cur.u - cpu->sav.u);
  s_frme = TRIMz(cpu->cur.s - cpu->sav.s);
  n_frme = TRIMz(cpu->cur.n - cpu->sav.n);
  i_frme = TRIMz(cpu->cur.i - cpu->sav.i);
  w_frme = TRIMz(cpu->cur.w - cpu->sav.w);
  x_frme = TRIMz(cpu->cur.x - cpu->sav.x);
  y_frme = TRIMz(cpu->cur.y - cpu->sav.y);
  z_frme = TRIMz(cpu->cur.z - cpu->sav.z);
  tot_frme = u_frme + s_frme + n_frme + i_frme + w_frme + x_frme + y_frme + z_frme;
#ifndef CPU_ZEROTICS
  if (tot_frme < cpu->edge)
    tot_frme = u_frme = s_frme = n_frme = i_frme = w_frme = x_frme = y_frme = z_frme = 0;
#endif
  if (1 > tot_frme) i_frme = tot_frme = 1;
  scale = 100.0 / (float)tot_frme;

   /* display some kinda' cpu state percentages
      (who or what is explained by the passed prefix) */

  // always here
  float pct_user = (float)(u_frme + n_frme) * scale,
          pct_syst = (float)(s_frme + x_frme + y_frme) * scale;

  return pct_user + pct_syst;
  #undef TRIMz
} // end: cpu_tics

        /*
         * In support of a new frame:
         *    1) Display uptime and load average (maybe)
         *    2) Display task/cpu states (maybe)
         *    3) Display memory & swap usage (maybe) */
float summary_show (void) {
  char tmp[MEDBUFSIZ];
  int i;
  float sum = 0.0f;

  cpus_refresh();

  for (i = 0; i < Cpu_faux_tot; i++) {
    sum += cpu_tics(&Cpu_tics[i], tmp, (i+1 >= Cpu_faux_tot));
  }

  return sum;
}

// Check the oversubscription periodically.
std::atomic<bool> stop_oversubs_chk(false);

void runOversubsChk() {
  cpuinfo();

  struct timespec ts = { cpu_util_chk_period, 0 };
  const int cpu_util_threshold = smp_num_cpus * 100 - cpu_util_spare;
  int print = 0;

  while (!stop_oversubs_chk) {
    pselect(0, NULL, NULL, NULL, &ts, NULL);
    float cpu_util = summary_show();

    if (cpu_util > cpu_util_threshold && print == 0) {
      logFail(__FILE__, __LINE__, "Current CPU utilization exceeds the threshold!");
      print = 1;
    }
  }
}