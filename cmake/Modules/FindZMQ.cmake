# Find the ZMQ libraries
#
# The following variables are optionally searched for defaults
#  ZMQ_ROOT:    Base directory where all ZMQ components are found
#
# The following are set after configuration is done:
#  ZMQ_FOUND
#  ZMQ_INCLUDE_DIR
#  ZMQ_LIBRARIES

find_path(ZMQ_INCLUDE_DIR NAMES zmq.h
          PATHS ${ZMQ_ROOT} ${ZMQ_ROOT}/include)

find_library(ZMQ_LIBRARIES NAMES zmq
             PATHS ${ZMQ_ROOT} ${ZMQ_ROOT}/lib)

if(NOT ZMQ_INCLUDE AND NOT ZMQ_LIBRARIES)
  #
  # Retry using pck-config:
  #
  find_package(PkgConfig)

  pkg_check_modules(PC_LIBZMQ libzmq)

  find_path(ZMQ_INCLUDE_DIR NAMES zmq.h
            PATHS ${PC_LIBZMQ_INCLUDE_DIRS})

  find_library(ZMQ_LIBRARIES NAMES zmq
               PATHS ${PC_LIBZMQ_LIBDIR} ${PC_LIBZMQ_LIBRARY_DIRS})

  set(ZMQ_VERSION ${PC_LIBZMQ_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZMQ DEFAULT_MSG ZMQ_LIBRARIES ZMQ_INCLUDE_DIR)

if(ZMQ_FOUND)
  mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_LIBRARIES)
endif()
