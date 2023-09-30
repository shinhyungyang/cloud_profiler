
find_package(SWIG REQUIRED)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
find_package(SQUASH)
find_package( Boost REQUIRED COMPONENTS system )
include(UseJava)
include(UseSWIG)
include_directories(${JNI_INCLUDE_DIRS} ${SQUASH_INCLUDE_DIR})
link_libraries(${SQUASH_LIBRARY})

set(CLOUD_PROFILER_SRC_FILES
    channels.cpp
    closure.cpp
    handlers.cpp
    log.cpp
    time.cpp
    time_io.cpp
    tsc/tsc.cpp
    os/cpu_util.cpp
    handlers/compression_thread/comp_thread.cpp
    handlers/io_thread/cleanup.cpp
    handlers/io_thread/io_thread.cpp
    handlers/io_thread/block_data.cpp
    handlers/id_handler.cpp
    handlers/buffer_id_handler.cpp
    handlers/null_handler.cpp
    handlers/downsample_handler.cpp
    handlers/firstlast_handler.cpp
    handlers/XoY_handler.cpp
    handlers/median_handler.cpp
    handlers/periodic_counter_handler.cpp
    handlers/fast_periodic_counter_handler.cpp
    handlers/defect_handler.cpp
    handlers/fox_start_handler.cpp
    handlers/fox_end_handler.cpp
    handlers/gpiortt_handler.cpp
    java_agent/JavaAgentHandler.cpp
    rtt.cpp
    net.c
    util.c
    cpp_emit/emit.cpp
)

if(CP_STATIC_BUILD)
  add_library(cloud_profiler_static STATIC ${CLOUD_PROFILER_SRC_FILES})
endif(CP_STATIC_BUILD)

set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/cp/swig)
include_directories(./)

set(CMAKE_SWIG_FLAGS -package cp.swig)

set_property(SOURCE cloud_profiler.i PROPERTY CPLUSPLUS ON)
set_property(SOURCE cloud_profiler.i PROPERTY SWIG_MODULE_NAME cloud_profiler)
SWIG_ADD_LIBRARY(
  cloud_profiler
  TYPE SHARED 
  LANGUAGE java
  SOURCES
    cloud_profiler.i
    ${CLOUD_PROFILER_SRC_FILES}
)
target_include_directories(
  cloud_profiler
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
# Enables -DZMQ_BUILD_DRAFT_API for using ZMQ Draft APIs
#set(ENABLE_DRAFTS ON)
target_include_directories(cloud_profiler PUBLIC ${ZMQ_INCLUDE_DIR})
target_include_directories(cloud_profiler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/config_server)
target_include_directories(cloud_profiler PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(cloud_profiler PUBLIC net_conf)
target_link_libraries(cloud_profiler PUBLIC ${Boost_LIBRARIES})
# Throttle GCC to avoid warnings with release build.
# ${swig_generated_file_fullname} is only defined *after* the call to
# SWIG_ADD_LIBRARY().
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_source_files_properties(${swig_generated_file_fullname}
                              PROPERTIES COMPILE_FLAGS "-fno-strict-aliasing")
endif()

ADD_CUSTOM_COMMAND(TARGET cloud_profiler POST_BUILD
    COMMAND "${Java_JAVAC_EXECUTABLE}" cp/swig/*.java
    COMMAND "${Java_JAVAC_EXECUTABLE}" ${CP_SRCDIR}/java_agent/*.java -d .
    COMMAND "${Java_JAVAC_EXECUTABLE}" ${CP_SRCDIR}/java/*.java -d .
    COMMAND "${Java_JAR_EXECUTABLE}"   -cfM cloud_profilerJNI.jar
                                       cp/swig/*.class  
                                       cp/java_agent/*.class
                                       cp/java/*.class
)

install(TARGETS cloud_profiler DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/src/cp/cloud_profilerJNI.jar DESTINATION lib)

if(CP_CONFIG_TSC)
  add_subdirectory(tsc)
endif(CP_CONFIG_TSC)

if(CP_CONFIG_ARMV8_CLOCK_TEST)
add_subdirectory(armfreq)
endif(CP_CONFIG_ARMV8_CLOCK_TEST)

add_subdirectory(config_server)
add_subdirectory(fox_clients)
add_subdirectory(ntplogd)
