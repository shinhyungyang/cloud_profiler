#ifndef __DEFECTHANDLER_H__
#define __DEFECTHANDLER_H__ // include-guard to prevent double-inclusion

#include "../closure.h"

#include <cstdint>
#include <string>

// A defect-handler is used with channels which already fail during the
// call to openChannel(). NET_CONF channels which are not able to obtain their
// configuration at creation time are the most likely candidates to use
// a defect-handler.
//
// The sole purpose of a defect-handler is respond to channel activities
// such as logTS and parameterizeChannel with a log-entry that flags the
// use of a channel which is in an error-state. To avoid flooding of log
// files, only the first invocation of each channel activity is logged.
//
// Rationale: the alternative to defect-handlers is to return an error value
// upon openChannel(). However, there is no way to enforce the client to check
// the error value. A client missing the error-value will use the error-vale
// as the closure pointer duing the next invocation of a channel operation.
// Casting -1 to a closure leads to nasty seg-faults.

void DefectHandler(void * cl, int64_t tuple);
// Discards all log information and warns that this channel is broken.

closure * initDefectHandler(std::string ch_name);
// Set up and return the closure of a defect_handler.

#endif
