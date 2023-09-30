#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "cxxopts.hpp"

#define	GPIO_REG_MAP            0xFF634000  // GPIO_REG_MAP
#define FSEL_OFFSET             0x116       // GPIOX_FSEL_REG_OFFSET
#define OUTP_OFFSET             0x117       // GPIOX_OUTP_REG_OFFSET
#define INP_OFFSET              0x118       // GPIOX_INP_REG_OFFSET
#define BLOCK_SIZE              (4*1024)

static void *map_base;
static volatile uint32_t *gpio;

#define PIN_LINE_M   8  // output on M, input on C
#define PIN_LINE_C   9  // output on C, input on M
#define PIN_BARR_M  10  // output on M, input on C
#define PIN_BARR_C  11  // output on C, input on M
#define LOW(target, pin) (!((target) & (1<<(pin))))
#define PIN(target, pin) ((target>>pin) & 1UL)

#define ITERATIONS (1000 * 1000 * 5)
#define UNROLLING_FACTOR 1
#define CONFIRM_ITER 64

uint64_t elapsed[ITERATIONS];
uint64_t start, end;


int main(int argc, char **argv) {
    time_t t;
    srand((unsigned) time(&t));
    uint32_t one = (uint32_t)(((double) rand() / (RAND_MAX)) + 1);
    uint32_t confirm_iter = one * CONFIRM_ITER;
    uint32_t each_iter = 0;

    bool isMaster = false;
    cxxopts::Options options("armfreq",
        "cloud_profiler ARMv8 counter measurement");
    options.add_options()
    //("c,console",     "Write log-file output to console",
    // cxxopts::value<bool>(console_log))
    //("i,iteration",   "The number of measurements to do",
    // cxxopts::value<long>(iteration))
      ("m,master", "Run as a master")
      ("c,client", "Run as a client")
      ("i,init",   "Initialize GPIO pins to input and quit")
      ("r,reset",  "Reset GPIO outputs to high")
      ("p,print",  "Print pin values")
      ("h,help", "Usage information")
      ("positional",
       "Positional arguments: these are the arguments that are entered "
       "without an option", cxxopts::value<std::vector<std::string>>())
      ;

    options.parse(argc, argv);

    if (options.count("h")) {
      std::cout << options.help({"", "Group"}) << std::endl;
      exit(EXIT_FAILURE);
    }

    if (options.count("m")) {
      isMaster = true;
    } else if (options.count("c")) {
      isMaster = false;
    }

    int fd;
    if ((fd = open("/dev/gpiomem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
      printf("Unable to open /dev/gpiomem. Here is possible solution:\n\n");
      printf("    (1) Double-check for a root-access\n");
      printf("    (2) Add username to a group with an access to the file.\n");
      printf("        See /etc/udev/rules.d/10-odroid.rules\n");
      return -1;
    }

    map_base = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    GPIO_REG_MAP);
    gpio = static_cast<volatile uint32_t *>(map_base);
    if (gpio < 0) {
        printf("Mmap failed.\n");
        close(fd);
        return -1;
    }

    if (options.count("p")) {
      // Print GPIOX FSEL register
      printf("GPIOX_FSEL register : 0x%08x\n",
             *(unsigned int *)(gpio + (FSEL_OFFSET)));

      // Print GPIOX OUTP register
      printf("GPIOX_OUTP register : 0x%08x\n",
             *(unsigned int *)(gpio + (OUTP_OFFSET)));

      munmap(map_base, BLOCK_SIZE);
      close(fd);
      return -1;
    }

    if (options.count("i")) {
      // Reset GPIOX FSEL register to make all pins input
      *(gpio + (FSEL_OFFSET)) |= 0xFFFFFFFF;

      printf("Reset all GPIO pins to input\n");
      
      // Print GPIOX FSEL register
      printf("GPIOX_FSEL register : 0x%08x\n",
             *(unsigned int *)(gpio + (FSEL_OFFSET)));

      munmap(map_base, BLOCK_SIZE);
      close(fd);
      return -1;
    }

    if ((options.count("m") == 0) && (options.count("c") == 0)) {
      printf("ERROR: please specifiy master or client.\n");

      munmap(map_base, BLOCK_SIZE);
      close(fd);
      return -1;
    }

    // Set all pins input
    *(gpio + (FSEL_OFFSET)) |= 0xFFFFFFFF;

    // Set direction of master pins to output
    if (isMaster) {
      *(gpio + (FSEL_OFFSET)) &= ~(1 << PIN_LINE_M);
      *(gpio + (FSEL_OFFSET)) &= ~(1 << PIN_BARR_M);
    } else {
      *(gpio + (FSEL_OFFSET)) &= ~(1 << PIN_LINE_C);
      *(gpio + (FSEL_OFFSET)) &= ~(1 << PIN_BARR_C);
    }

    // Reset and quit so that inputs of opponent will be initialized to an
    // expected value
    if (options.count("r")) {

      if (isMaster) {
        *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_M);  // low
        *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_M);  // low
      } else {
        *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_C);  // low
        *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_C);  // low
      }

      munmap(map_base, BLOCK_SIZE);
      close(fd);
      return -1;
    }

    for (int i=0; i<ITERATIONS; ++i) {
      elapsed[i] = 0UL;
    }

    // init GPIO lines to 0 (low):
    if (isMaster) {
      *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_M);  // low
      *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_M);  // low
    } else {
      *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_LINE_C);  // low
      *(gpio + (OUTP_OFFSET)) &= ~(1 << PIN_BARR_C);  // low
    }

    uint32_t barr_m = 0;
    uint32_t barr_c = 0;
    uint32_t line_m = 0;
    uint32_t line_c = 0;

    if (UNROLLING_FACTOR == 1) {

      if (isMaster) {

        barr_c = !barr_c; // sense reverse
        // Wait for client to get ready
        while (PIN((*(gpio + (INP_OFFSET))),PIN_BARR_C) != barr_c) {;}
        barr_m = !barr_m; // sense reverse
        // Signal client to start experiment (barr_m not used; only toggling)
        *(gpio + (OUTP_OFFSET)) ^= (1UL << PIN_BARR_M);

        for (int i=0; i<ITERATIONS; ++i) {

          line_c = !line_c; // sense reverse
          line_m = !line_m; // sense reverse

          // Wait for client to signal
          while (PIN((*(gpio + (INP_OFFSET))),PIN_LINE_C) != line_c) {;}
          // Signal back to client (line_m not used; only toggling)
          *(gpio + (OUTP_OFFSET)) ^= (1UL << PIN_LINE_M);
        }
      }
      else { // Client

        barr_c = !barr_c; // sense reverse
        barr_m = !barr_m; // sense reverse
        // Signal master to confirm client is ready
        *(gpio + (OUTP_OFFSET)) ^= (1UL << PIN_BARR_C);
        // Wait for server to get ready
        while (PIN((*(gpio + (INP_OFFSET))),PIN_BARR_M) != barr_m) {;}

        for (int i=0; i<ITERATIONS; ++i) {

          line_c = !line_c; // sense reverse
          line_m = !line_m; // sense reverse

          asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(start));

          // Signal master (line_c not used; only toggling)
          *(gpio + (OUTP_OFFSET)) ^= (1UL << PIN_LINE_C);
          // Wait for master to signal back
          while (PIN((*(gpio + (INP_OFFSET))),PIN_LINE_M) != line_m) {;}

          asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(end));

          elapsed[i] = end-start;
        }
      }
    }

    if (!isMaster) {
      for (int i=0; i<ITERATIONS; ++i) {
        printf("%u\n", elapsed[i]);
      }
    }

    *(gpio + (FSEL_OFFSET)) |= 0xFFFFFFFF;
    munmap(map_base, BLOCK_SIZE);
    close(fd);
    return 0;
}
