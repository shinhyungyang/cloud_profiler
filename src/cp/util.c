#define _GNU_SOURCE // define drand48_r()
#define _ISOC11_SOURCE // define aligned_alloc()

#include "time_tsc.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

void Usage(int argc, char * argv[], char * extra) {
  fprintf(stderr, "Usage: %s -i number_of_iterations", argv[0]);
  if (extra != NULL) {
    fprintf(stderr, " %s", extra);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "number_of_iterations accepts postfix modifiers: "
                  "KB, KiB, MB, MiB, GB, GiB\n");
  exit(EXIT_FAILURE);
}

void fail(char *file, int line, char *call, int retval ) {
  fprintf(stdout, "ERROR: %s FAILED, Line # %d, File %s\n", call, line, file);
  exit(EXIT_FAILURE);
}

char * l2aCommas(long long num, char *ptr) {
  long long n = num<0?-num:num;
  int negative = num<0?1:0;
  int cur_digit, digit_nr = 1, index = 0;
  while(1) {
    cur_digit = n % 10; 
    ptr[index++] = cur_digit + '0'; // Add ASCII offset for '0'.
    n = n / 10;
    if (n == 0)
      break;
    if (digit_nr % 3 == 0)
      ptr[index++] = ',';
    digit_nr++;
  }
  if (negative) {
    ptr[index++] = '-';
  }
  ptr[index] = '\0';
  // Mirrow in-place:
  for (int low = 0, high = index-1; low < high; low++, high--) {
    char temp = ptr[low];
    ptr[low]  = ptr[high];
    ptr[high] = temp;
  }
  return ptr;
}

//FIXME: this function should be expressed in terms of
// convertA2Inoexit (below) to avoid code duplication.
unsigned long long convertA2I(const char * str) {
  char * endptr;
  unsigned long long ret = strtoull(str, &endptr, 10);
  if (endptr == str) {
    fprintf(stderr, "Invalid number: %s\n", str);
    exit(EXIT_FAILURE);
  }
  if (*endptr == '\0') {
    // entire string is valid:
    return ret;
  }
  // Look for modifiers:
  unsigned long long factor = 1;
  if (0==strcmp(endptr, "KB")) {
    factor = 1000;
  } else if (0==strcmp(endptr, "KiB")) {
    factor = 1024;
  } else if (0==strcmp(endptr, "MB")) {
    factor = 1000000;
  } else if (0==strcmp(endptr, "MiB")) {
    factor = 1048576;
  } else if (0==strcmp(endptr, "GB")) {
    factor = 1000000000;
  } else if (0==strcmp(endptr, "GiB")) {
    factor = 1073741824;
  } else {
    fprintf(stderr, "Invalid number postfix: %s\n", str);
    exit(EXIT_FAILURE);
  }
  return ret * factor;
}

void convertA2Inoexit(const char * str, struct conv_result * r) {
  char * endptr;
  r->valid = -1;
  r->result = strtoull(str, &endptr, 10);
  if (endptr == str) {
    snprintf(r->err_msg, 1024, "Invalid number: %s", str);
    return;
  }
  if (*endptr == '\0') {
    // entire string is valid:
    r->valid = 1;
    return;
  }
  // Look for modifiers:
  unsigned long long factor = 1;
  if (0==strcmp(endptr, "KB")) {
    factor = 1000;
  } else if (0==strcmp(endptr, "KiB")) {
    factor = 1024;
  } else if (0==strcmp(endptr, "MB")) {
    factor = 1000000;
  } else if (0==strcmp(endptr, "MiB")) {
    factor = 1048576;
  } else if (0==strcmp(endptr, "GB")) {
    factor = 1000000000;
  } else if (0==strcmp(endptr, "GiB")) {
    factor = 1073741824;
  } else {
    snprintf(r->err_msg, 1024, "Invalid number postfix: %s", str);
    return;
  }
  r->result = r->result * factor;
  r->valid = 1;
}

int init_rand(struct drand48_data * data) {
  struct timespec time;
  int result = clock_gettime(CLOCK_REALTIME, &time);
  if (result == -1) {
    return -1;
  }
  srand48_r(time.tv_nsec, data);
  return 0;
}

int within(struct drand48_data * data, int num) {
  double rand_val = 0;
  drand48_r(data, &rand_val);
  rand_val = floor(rand_val * (double)num);
  return (int)rand_val;
}

