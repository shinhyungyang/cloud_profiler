#ifndef __UTIL_H__
#define __UTIL_H__ // prevent against double-inclusion

#include <stdlib.h>

char *l2aCommas(long long num, char *ptr);
// Convert num to string, grouping into groups of three digits,
// seperated by commas. It's the user's repsonsibility to provide
// enough space in ``ptr''. The returned string is terminated by \0.

unsigned long long convertA2I(const char * str);
// Convert string to integer. Supports postfix qualifiers
// KB, KiB, MB, MiB, GB, GiB. E.g., 10KB stands for 1000 bytes,
// 10KiB stands for 1024 bytes.

struct conv_result {
  unsigned long long result;
  char valid; // -1 in error case, 1 else
  char err_msg[1024];
};

void convertA2Inoexit(const char * str, struct conv_result * r);
// Same as convertA2I, except that the function does not terminate if
// "str" does not contain a valid numeric literal.
// In this (error) case, "valid" is set to -1, and err_msg[] contains
// an error string.

void Usage(int argc, char * argv[], char * extra);

void fail(char *file, int line, char *call, int retval);

int init_rand(struct drand48_data * data);

int within(struct drand48_data * data, int num);

#endif
