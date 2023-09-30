#ifndef __EXPORTS_H__
#define __EXPORTS_H__

#include "tsc_messages.h"
#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT_GUID(connect_req, "connect_req")

BOOST_CLASS_EXPORT_GUID(register_req, "register_req")       // slave -> master
BOOST_CLASS_EXPORT_GUID(register_accept, "register_accept") // master -> slave

BOOST_CLASS_EXPORT_GUID(measure_req, "measure_req") // master -> slave
BOOST_CLASS_EXPORT_GUID(meas_req_accept, "meas_req_accept")   // slave -> master

#if 0
BOOST_CLASS_EXPORT_GUID(measure_info, "measure_info")   // master -> slave
BOOST_CLASS_EXPORT_GUID(info_received, "info_received") // slave -> master

BOOST_CLASS_EXPORT_GUID(measure_result, "measure_result") // slave -> master
BOOST_cLASS_EXPORT_GUID(result_received, "result_received") // master -> slave
#endif

#endif
