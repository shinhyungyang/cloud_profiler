#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define	GPIO_REG_MAP            0xFF634000
#define GPIOX_FSEL_REG_OFFSET   0x116
#define GPIOX_OUTP_REG_OFFSET   0x117
#define GPIOX_INP_REG_OFFSET    0x118
#define BLOCK_SIZE              (4*1024)

static volatile uint32_t *gpio;

#define GPIOX_PIN_OUTPUT  9  // OUTPUT: GPIOX.9
#define GPIOX_PIN_INPUT  11  // INPUT:  GPIOX.11
#define LOW(target, pin) (!((target) & (1<<(pin))))

#define ITERATIONS (1000 * 1000 * 100)
#define UNROLLING_FACTOR 1

int main(int argc, char **argv) {
    int fd;
    if ((fd = open("/dev/gpiomem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
        printf("Unable to open /dev/gpiomem\n");
        return -1;
    }

    gpio = mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_REG_MAP);
    if (gpio < 0) {
        printf("Mmap failed.\n");
        return -1;
    }

    // Reset GPIOX FSEL register to make all pins input
    *(gpio + (GPIOX_FSEL_REG_OFFSET)) |= 0xFFFFFFFF;

    // Set direction of GPIOX_PIN_OUTPUT register to out
    *(gpio + (GPIOX_FSEL_REG_OFFSET)) &= ~(1 << GPIOX_PIN_OUTPUT);

    // Set GPIOX_PIN_OUTPUT to high
    *(gpio + (GPIOX_OUTP_REG_OFFSET)) |= (1 << GPIOX_PIN_OUTPUT);
    // and wait until input receives high
    while (LOW((*(gpio + (GPIOX_INP_REG_OFFSET))),(GPIOX_PIN_INPUT))) ;

    uint64_t elapsed;
    uint64_t start, end;

    elapsed = 0UL;

    if (UNROLLING_FACTOR == 1) {

      asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(start));

      for (int i=0; i<ITERATIONS; ++i) {

        // Set GPIOX_PIN_OUTPUT to low
        *(gpio + (GPIOX_OUTP_REG_OFFSET)) &= ~(1 << GPIOX_PIN_OUTPUT);
        // and wait until input receives low
        while (!LOW((*(gpio + (GPIOX_INP_REG_OFFSET))),(GPIOX_PIN_INPUT))) ;

        // Set GPIOX_PIN_OUTPUT to high
        *(gpio + (GPIOX_OUTP_REG_OFFSET)) |= (1 << GPIOX_PIN_OUTPUT);
        // and wait until input receives high
        while (LOW((*(gpio + (GPIOX_INP_REG_OFFSET))),(GPIOX_PIN_INPUT))) ;

      }

      asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(end));

      elapsed = end-start;

    }

    printf("%lu\n", elapsed);

    return 0;
}
