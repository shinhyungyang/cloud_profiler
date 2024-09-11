#ifndef __TIME_IO_H__
#define __TIME_IO_H__ // prevent against double-inclusion

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

void writeTimeStampedCtr(std::ostream & outf, struct timespec & ts, 
                    uint64_t ctr, bool newline);

void writeTimeStampHeader(std::ostream & outf);

#endif
