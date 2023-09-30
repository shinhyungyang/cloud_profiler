#include <sys/time.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv) {
        time_t test;
        printf("Size of struct timeval: %lu bytes\n", sizeof(struct timeval));
        printf("Size of time_t: %lu bytes\n", sizeof(test));
        printf("Size of long: %lu bytes\n", sizeof(long));
        printf("Size of clockid_t: %lu bytes\n", sizeof(clockid_t));
        return 0;
}
