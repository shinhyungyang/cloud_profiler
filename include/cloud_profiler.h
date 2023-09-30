#ifndef __CLOUD_PROFILER_H__
#define __CLOUD_PROFILER_H__ // prevent against double-inclusion

#include "java_agent/JavaAgentHandler.h"

#include <cstdint>
#include <string>
#include <vector>
#include <list>


enum log_format { ASCII, ASCII_TSC, BINARY, BINARY_TSC,
                  ZSTD, ZSTD_TSC, LZO1X, LZO1X_TSC };
// Format in which data is written to the log files.

enum handler_type { IDENTITY, BUFFER_IDENTITY, BUFFER_IDENTITY_COMP,
                    MEDIAN, DOWNSAMPLE, XOY, FIRSTLAST,
                    PERIODICCOUNTER, FASTPERIODICCOUNTER, NULLH, NET_CONF,
                    FOX_START, FOX_END, GPIORTT };

int64_t openChannel(std::string ch_msg, log_format f, handler_type h);
// Open a logging channel. Returns the channel ID (which is essentially a
// 64-bit pointer cast cast to int). The caller should regard the channel ID as
// an opaque type, and *not* alter it.
// The "ch_msg" becomes part of the channel's log file name.
// Blanks are replaced by underscores. Apart from blanks, the string should
// be valid as a substring in a filename. This function is thread-safe.
// To query the status of a channel, a caller must call the isOpen() method
// (see below).

bool isOpen(int64_t ch);
// Returns true if 'ch' is an opened channel, and false otherwise.
// Note 1: a channel may fail to open (e.g., if the channel configuration
// cannot be obtained via the network). In such a case isOpen() will return
// false.
// Note 2: even if isOpen() returns true, the channel might close
// asynchronously after the call to isOpen(). Thus, a succeeding isOpen() call
// does not guarantee that following operations on the channel will succeed.

void closeChannel(int64_t ch);
// Closes an open channel.

int parameterizeChannel(int64_t ch, int32_t arg_number, int64_t arg_value);
// Used to pass additional argument values to channel ch.
// arg_number specifies the nth additional argument (starting from 0!).
// value specifies the value passed for the argument.
// Returns -1 in case of error. (E.g., if this channel's handler does not
// support parameterization.) On error, the channel is closed.
// See the header file for a particular handler to find out about valid
// argument ranges.

int parameterizeChannel(int64_t ch,
                        int32_t arg_number,
                        const std::string arg_value);
// Overloaded function that allows to provide a string arg_value to a handler.
// The string must contain a numerical literal. The following postfix
// qualifiers are allowed: KB, KiB, MB, MiB, GB, GiB.
// E.g., 10KB stands for 1000 bytes, 10KiB stands for 1024 bytes.

void logTS(int64_t ch, int64_t tuple);
// Call the handler of channel "ch" with tuple "tuple". The handler itself
// defines the behavior of what is going to get logged.


int getTS(struct timespec *ts);
// Reads the current time and returns it in timespec ts. -1 is returned in
// case of error. This function is thread-safe.

void addTS(struct timespec *result,
           const struct timespec *t1,
           const struct timespec *t2);
// Add two timestamps. This function is thread-safe.

void subtractTS(struct timespec *result,
                struct timespec *start,
                struct timespec *end);
// Subtract two timestamps from each other. This function is thread-safe.

int divideTS(struct timespec *result, struct timespec * t, int div);
// Returns -1 in case of overflow.

int compareTS(const struct timespec * t1, const struct timespec * t2);
// Returns -1 if *t1 < *t2, 0 if *t1 == *t2 and 1 otherwise.

int formatTS(struct timespec *ts,
             char *s,
             size_t max,
             const std::string format);
// Format a time-stamp ts according to the format string f provided as an
// argument. See "man strftime" for format string specfications.
// The caller must provide the buffer in s (and max). In Java, buffer s must
// be a byte[] array. ''format'' must be a Java string.
// The caller is responsible that the size of s is large
// enough. If the buffer is too small, formatTS will return -1.

int formatTS(struct timespec *ts, char *s, size_t max);
// Format a time-stamp using the default format string "%Y-%m-%d_%H:%M:%S".

int64_t installAgentHandler(JavaAgentHandler* obj, long period_ms);
// Receives JavaAgentHandler object inherited from Java and period value
// for setting Java's garbage collection sampling period in milliseconds.
//
void uninstallAgentHandler();
// Requests to stop the GC sampling thread in preparation to exit from current
// process.

int formatDuration(struct timespec *ts, char *s, size_t max, bool units);
// Format a timespec as a duration rather than a date. If units = true,
// the units will be appended.

int formatDuration(uint64_t t_secs, uint64_t ns,
                   char *s, size_t max, bool units);
// Format a duration given in seconds and nano-seconds. Except the different
// specification of the time, this function is identical to the one above.

int formatDuration(uint64_t ns, char *s, size_t max, bool units);
// Format a duration given in ns. Except the different specification of the
// time, this function is identical to the one above.

void nologTS(int64_t ch, int64_t tuple);
// Empty call to the JNI. Used to determine the per call JNI overhead
// for our most frequently-used API function.

void printStringVector(std::vector<std::string> & node_list);
// Print std::string in the vector. Used to test the validity of the vector
// between Java and C++.

std::string * newString(std::string str1);
// Allocates std::string from the content of Java String argument. Its typemap
// definition is a crucial part of the actual task.

void printStrPtrVector(std::vector<std::string *> & strptrVec);
// Print the content of std:;string * vector. Used to test the validity of the
// vector between Java and C++.

void deleteStrPtrVector(std::vector<std::string *> & strptrVec);
// Deallocates all std::string pointers in the vector, because SWIG-Java does
// not support automated clean-up of user-defined allocations.

void addToStrPtrVec(std::vector<std::string *> & strptrVec, std::string str2);
// Add string to the vector passed as an argument. Used as an example which do
// not use customized typemap but generates a default SWIG wrapper

int64_t openTCPChannel(bool isServer, std::vector<std::string> & addresses,
                       int64_t nr_threads);
//
//

void closeTCPChannel(int64_t ch);
//
//

void initDGSender(JavaAgentHandler* obj,
                  int64_t sv_port_nr, int64_t emit_dur, int64_t pill_dur,
                  int64_t tup_idx_offset,
                  int64_t record_dur, int64_t per_sec_throughput, int64_t slag,
                  int64_t nr_threads);
// Initializes DG sender

void runDGSender();
// Runs DG sender and deallocate all resources at the end

long loadOneTuple(char* s, size_t length);
// Prepare a tuple as a byte array. It returns the number of remaining tuples
// to be created

void loadPoisonPills(char* s, size_t length);
// Prepare poison pills

void loadAllStrTuples();
// Prepare all std::string tuples automatically with the upcall get_tuple()

int64_t emit(JavaAgentHandler* obj, int64_t ch, int64_t emitDuration, int64_t emitThroughput,
    int64_t tupleIdx, int64_t tupleIdxOffset, int64_t timeBudget);
//
//

const char* recvStr(int64_t ch);
//
//

const char* fetchStr(int64_t ch);
//
//

int64_t openSelfFeedChannel(int64_t emit_dur, int64_t per_sec_throughput,
    int64_t slag, int64_t nr_threads);
//
//

int64_t prepareFetchingTest();
// Prepares a fetching test. It allocates a fetching_test_closure for storing
// several bookkeepings including cloud_profiler logging channels

char* fetchCStr();
// Returns a pointer to a char buffer which contains a premade sample event.
// With the Swig layer, it generates a Java string using typemap(out) char*.

int64_t fetchInt(int64_t ch);
// Returns an integer primitive to trigger the maximum ingestion rate of the
// target system

int64_t openZMQChannel(std::string address);
//
//

int64_t openZMQBucketChannel(std::string address);
// Works the same as openZMQChannel except for the additional socket option
// which sets non-blocking socket receive.

int64_t openZMQBktOptChannel(std::string address);
// Works the same as openZMQBucketChannel except for not initializing timer
// variables

void closeZMQChannel(int64_t ch);
//
//

char* fetchZMQStr(int64_t ch);
// Returns a pointer to a char buffer which contains a received event.
// With the Swig layer, it generates a Java string using typemap(out) char*.

char* fetchZMQStrNB(int64_t ch);
// Works the same as fetchZMQStr except for the non-blocking receive which
// returns NULL if nothing is received.

void* fetchZMQStrNB3(int64_t ch, int64_t msg);
// Exports fetchZMQStrNB2

int fetchZMQBucket(int64_t chBucket, int tuningKnob, char** ZMQBucket);
//
//

int fetchZMQBktOpt(int64_t chBucket, int tuningKnob, int tid, char** ZBktOpt);
// Fills in a bucket with Java strings marshalled with newly received tuples

int fetchObjBktOpt(int64_t chBucket, int tuningKnob, int tid, char** byte2D);
// Fills in a bucket with byte arrays of newly received tuples

void fetchCStrBucket(char** CStrBucket);
//
//

void initTimeoutVars(int64_t ch, int64_t nr_threads);
//
//

void logFail(const char *file, int line, const std::string & msg);
//
//

#endif
