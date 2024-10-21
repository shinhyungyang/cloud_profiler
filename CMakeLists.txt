cmake_minimum_required(VERSION 3.15.1 FATAL_ERROR)
# (for SWIG_ADD_LIBRARY)

cmake_policy(SET CMP0078 NEW) # ``UseSWIG`` generates standard target names
cmake_policy(SET CMP0086 NEW) # Create and obey ``-module'' argument

if(CMAKE_VERSION VERSION_LESS "3.27.0")
   cmake_policy(SET CMP0074 NEW) # Obey <Packge>_ROOT hints
else if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
   cmake_policy(SET CMP0144 NEW) # Obey <PACKAGE>_ROOT hints
endif()

project(cloud_profiler
        VERSION 0.3.2
        DESCRIPTION "Cloud instrumentation and profiling library"
)

option(CP_STATIC_BUILD
       "Add static library of the cloud_profiler to the build." OFF)

option(CP_CONFIG_SERVER
	"Build the configuration server." ON)

option(CP_CONFIG_TSC
       "TSC-related features." ON)

option(CP_CONFIG_JLISTS
       "SWIG-Java feature for supporting C++ Vector and List" ON)

option(CP_CONFIG_ZMQ_TEST
      "Profile ZMQ performance using cloud_profiler handlers" ON)

option(CP_CONFIG_CPP_SINK
      "Run C++Sink which receives data from data generators " ON)

option(CP_CONFIG_TSC_TIMESPEC
       "Extended timespec and clock_gettime" ON)

option(CP_CONFIG_ARMV8_CLOCK_TEST
      "Test CPU clock frequency on ARMv8 devices" OFF)

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
# Require C++ 11 standard, no fallback, no extensions.


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(EnableCompilerFlags REQUIRED)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

if(CP_CONFIG_SERVER)
  find_package(ZMQ REQUIRED)
endif(CP_CONFIG_SERVER)

if(CP_CONFIG_TSC OR CP_CONFIG_TSC_TIMESPEC)
  find_package(PAPI)
endif()

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64.*")
  set(CP_CONFIG_ARMV8_CLOCK_TEST ON)
else()
  set(CP_CONFIG_ARMV8_CLOCK_TEST OFF)
endif()
message(STATUS "CP_CONFIG_ARMV8_CLOCK_TEST: ${CP_CONFIG_ARMV8_CLOCK_TEST}")

if(CMAKE_COMPILER_IS_GNUCC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.1)
    message(FATAL_ERROR "Require at least gcc-7.1")
  else()
    # Enable "over-alignment", e.g., on cache-line granularity:
    enable_cxx_compiler_flag_if_supported("-faligned-new")
  endif()
endif()
enable_c_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
#enable_cxx_compiler_flag_if_supported("-pedantic")

#
# Make installed libraries self-contained:
#

# (1) reference our own installed libraries:
list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

# (2) add the paths of the libraries used during compilation:
# Disable if the cloud_profiler libraries are re-located to a node
# with different library locations:
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include(BuildType)

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CP_BINDIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CP_BINDIR})

add_subdirectory(src)   

install(FILES include/cloud_profiler.h DESTINATION include)
