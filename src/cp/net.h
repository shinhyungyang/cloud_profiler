#ifndef __NET_H__
#define __NET_H__ // prevent against double-inclusion

#include <string.h>

int getIPDottedQuad(char *s, size_t max);
// Determine the IP address in dotted quad format (e.g., 192.168.1.1)
// of the computer we're running on.
// Return value: 0 in case of success, -1 for errors. In the error-case,
// the returned IP address string is set to a default value: 0.0.0.0.

#endif
