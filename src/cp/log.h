#ifndef __LOG_H__
#define __LOG_H__ // prevent against double-inclusion

#include <string>

void openStatusLogFile(void);
// Open the cloud profiler's status log file.
// This operation is idempotent.

void closeStatusLogFile(void);
// Close the cloud profiler's status log file.
// This operation is idempotent.

void logFail(const char *file, int line, const std::string & msg);
// Log a failure (thread-safe).

void logFail(const char *file, int line, const std::string & msg, int err);
// Log a failure, including error-value (thread-safe).

void logStatus(const char *file, int line, const std::string & msg);
// Log a status message (thread-safe).

void logStatus(const char *file, int line, const std::string & msg, int err);
// Log a status message, including error-value (thread-safe).

void toggleConsoleLog(bool flag);
// Control logging of output to the console (stderr).
// flag == true -> console log is on; off otherwise.

void setLogFileName(const char * name);
// Explicity set the name of the output log file.
// (This module otherwise uses its default name.)

#endif
