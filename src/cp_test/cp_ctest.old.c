#define __USE_POSIX // for localtime_r
#define _POSIX_C_SOURCE 199309
#include<stdio.h>
#include<time.h>
#include<sys/time.h>
#include<assert.h>

int main(void) {

  struct timeval t1, t2;
  int ret = gettimeofday(&t1, NULL);
  assert (ret==0);
  ret = gettimeofday(&t2, NULL);
  assert (ret==0);
  printf("t1: %ld:%ld\n", t1.tv_sec, t1.tv_usec);
  printf("t2: %ld:%ld\n", t2.tv_sec, t2.tv_usec);
  struct tm now; char tmp_buf[1024], buf[1024];
  localtime_r(&t1.tv_sec, &now);
  strftime(tmp_buf, sizeof(tmp_buf), "%Y-%m-%d %H:%M:%S", &now);
  snprintf(buf, sizeof(buf), "%s.%06ld", tmp_buf, t1.tv_usec);
  printf("now: %s\n", buf);

  struct timespec clock_res;
  ret = clock_getres(CLOCK_REALTIME, &clock_res);
  assert (ret == 0);
  printf("CLOCK_REALTIME resolution: %ld ns\n", clock_res.tv_nsec);
  ret = clock_getres(CLOCK_REALTIME, &clock_res);
  assert (ret == 0);
  printf("CLOCK_REALTIME resolution: %ld ns\n", clock_res.tv_nsec);

  return 0;
}
