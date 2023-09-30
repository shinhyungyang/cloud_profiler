#define _ISOC11_SOURCE // define aligned_alloc()

#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

static char IP[255]; // Per process ``cache''
static int IP_valid = 0;
static pthread_mutex_t IPlock = PTHREAD_MUTEX_INITIALIZER;

// Adopted from
// https://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
//
int getIPDottedQuad(char *s, size_t max) {
  struct ifaddrs * ifAddrStruct=NULL;
  struct ifaddrs * ifa=NULL;
  void * tmpAddrPtr=NULL;
  char error_IP[] = "0.0.0.0";
  int found = 0;
  pthread_mutex_lock(&IPlock);
  if (IP_valid) {
    strncpy(s, IP, max);
    pthread_mutex_unlock(&IPlock);
    return 0;
  }
  pthread_mutex_unlock(&IPlock);

  strncpy(s, error_IP, max); // Default

  if (0 != getifaddrs(&ifAddrStruct)) {
    return -1;
  }

  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) {
      continue;
    }
    if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
      // is a valid IP4 Address
      tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
      if (0 != strcmp("lo", ifa->ifa_name)) {
        // Non-local (127.*.*.*) IP address:
        strncpy(s, addressBuffer, max);
        found = 1;
        pthread_mutex_lock(&IPlock);
        if(!IP_valid) {
          //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
          strncpy(IP, addressBuffer, 255);
          IP_valid = 1;
        }
        pthread_mutex_unlock(&IPlock);
        break;
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
      // is a valid IP6 Address
      tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
      char addressBuffer[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
      //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
    } 
  }
  if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
  if (!found) {
    return -1;
  }
  return 0;
}
