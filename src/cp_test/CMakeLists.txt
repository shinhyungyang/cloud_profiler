
find_package(Threads REQUIRED)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
find_package(Boost REQUIRED)

include(UseJava)
#include_directories(${JNI_INCLUDE_DIRS})

# Configure jtest.sh.in and copy to output directory:
configure_file("jtest.sh.in" "${CP_BINDIR}/jtest.sh")
# Ditto for jkernels.sh:
configure_file("jkernels.sh.in" "${CP_BINDIR}/jkernels.sh")
# Yet another configuration for jlists.sh:
configure_file("jlists.sh.in" "${CP_BINDIR}/jlists.sh")
# Configure jstring.sh.in and copy to output directory:
configure_file("jstring.sh.in" "${CP_BINDIR}/jstring.sh")

#
# C++: cp_cpptest
#
add_executable(cp_cpptest cp_cpptest.cpp)
target_include_directories(cp_cpptest PUBLIC ../cxxopts/include)
target_link_libraries(cp_cpptest PRIVATE cloud_profiler)
target_link_libraries(cp_cpptest PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_cppbuffertest
#
add_executable(cp_cppbuffertest cp_cppbuffertest.cpp)
target_include_directories(cp_cppbuffertest PUBLIC ../cxxopts/include)
target_link_libraries(cp_cppbuffertest PRIVATE cloud_profiler)
target_link_libraries(cp_cppbuffertest PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_cppiotest
#
add_executable(cp_cppiotest cp_cppiotest.cpp)
target_include_directories(cp_cppiotest PUBLIC ../cxxopts/include)
target_link_libraries(cp_cppiotest PRIVATE cloud_profiler)
target_link_libraries(cp_cppiotest PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# GPIO test
#
add_executable(cp_gpiotest cp_gpiotest.cpp)
target_include_directories(cp_gpiotest PUBLIC ../cxxopts/include)
target_link_libraries(cp_gpiotest PRIVATE cloud_profiler)
target_link_libraries(cp_gpiotest PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_tsctest
#
add_executable(cp_tsctest cp_tsctest.cpp)
target_include_directories(cp_tsctest PUBLIC ../cxxopts/include)
target_link_libraries(cp_tsctest PRIVATE cloud_profiler)
target_link_libraries(cp_tsctest PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_valtsc
#
add_executable(cp_valtsc cp_valtsc.cpp)
target_include_directories(cp_valtsc PUBLIC ../cxxopts/include)
target_link_libraries(cp_valtsc PRIVATE cloud_profiler)
target_link_libraries(cp_valtsc PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_valtsc2
#
add_executable(cp_valtsc2 cp_valtsc2.cpp)
target_include_directories(cp_valtsc2 PUBLIC ../cxxopts/include)
target_link_libraries(cp_valtsc2 PRIVATE cloud_profiler)
target_link_libraries(cp_valtsc2 PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#
# C++: cp_calctsc
#
add_executable(cp_calctsc cp_calctsc.cpp)
target_include_directories(cp_calctsc PUBLIC ../cxxopts/include)
target_link_libraries(cp_calctsc PRIVATE cloud_profiler)
target_link_libraries(cp_calctsc PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# For convenience we copy the binaries to the bin folder:
add_custom_command(
  TARGET cp_cpptest
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_cpptest> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_cppbuffertest
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_cppbuffertest> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_cppiotest
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_cppiotest> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_gpiotest
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_gpiotest> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_tsctest
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_tsctest> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_valtsc
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_valtsc> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_valtsc2
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_valtsc2> ${CP_BINDIR}
)

add_custom_command(
  TARGET cp_calctsc
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:cp_calctsc> ${CP_BINDIR}
)

#
# Java: cp_jtest.jar
#
add_jar(
  cp_jtest
  SOURCES cp_jtest.java
  ENTRY_POINT cp_jtest
  INCLUDE_JARS ${CP_JAR} ${JOPT_JAR}
)
add_dependencies(cp_jtest cloud_profiler)

#
# Java: cp_jkernels.jar
#
add_jar(
  cp_jkernels
  SOURCES cp_jkernels.java
  ENTRY_POINT cp_jkernels
  INCLUDE_JARS ${CP_JAR} ${JOPT_JAR}
)
add_dependencies(cp_jtest cloud_profiler)

if(CP_CONFIG_JLISTS)

  #
  # Java: cp_jlists.jar
  #
  add_jar(
    cp_jlists
    SOURCES cp_jlists.java
    ENTRY_POINT cp_jlists
    INCLUDE_JARS ${CP_JAR} ${JOPT_JAR}
  )
  add_dependencies(cp_jlists cloud_profiler)
endif(CP_CONFIG_JLISTS)

#
# Java: cp_jstring.jar
#
add_jar(
  cp_jstring
  SOURCES cp_jstring.java
  ENTRY_POINT cp_jstring
  INCLUDE_JARS ${CP_JAR} ${JOPT_JAR}
)
add_dependencies(cp_jstring cloud_profiler)

if(CP_CONFIG_TSC)

  #
  # rdtsc_test
  #
  add_executable(rdtsc_test rdtsc_test.cpp)
  target_include_directories(rdtsc_test PRIVATE ${CP_SRCDIR})
  target_include_directories(rdtsc_test PRIVATE ${PAPI_INCLUDE_DIR})
  target_include_directories(rdtsc_test PUBLIC ../cxxopts/include)
  target_link_libraries(rdtsc_test PRIVATE cloud_profiler)
  target_link_libraries(rdtsc_test PRIVATE ${PAPI_LIBRARIES})
  target_link_libraries(rdtsc_test PRIVATE ${CMAKE_THREAD_LIBS_INIT})

  # For convenience we copy the binary to the bin folder:
  add_custom_command(
    TARGET rdtsc_test
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      -E copy_if_different $<TARGET_FILE:rdtsc_test> ${CP_BINDIR}
  )
endif(CP_CONFIG_TSC)

if(CP_CONFIG_TSC_TIMESPEC)

  #
  # cp_tstest
  #
  add_executable(cp_tstest cp_tstest.cpp)
  target_include_directories(cp_tstest PRIVATE ${CP_SRCDIR})
  target_include_directories(cp_tstest PRIVATE ${PAPI_INCLUDE_DIR})
  target_include_directories(cp_tstest PUBLIC ../cxxopts/include)
  target_link_libraries(cp_tstest PRIVATE cloud_profiler)
  target_link_libraries(cp_tstest PRIVATE ${PAPI_LIBRARIES})

  add_custom_command(
    TARGET cp_tstest
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      -E copy_if_different $<TARGET_FILE:cp_tstest> ${CP_BINDIR}
  )
endif(CP_CONFIG_TSC_TIMESPEC)

if(CP_CONFIG_ZMQ_TEST)

  #
  # cp_zmqtest
  #
  add_executable(cp_zmqtest cp_zmqtest.cpp)
  target_include_directories(cp_zmqtest PRIVATE ${CP_SRCDIR})
  target_include_directories(cp_zmqtest PRIVATE ../cppzmq)
  target_include_directories(cp_zmqtest PUBLIC ../cxxopts/include)
  target_include_directories(cp_zmqtest PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(cp_zmqtest PRIVATE ${CMAKE_THREAD_LIBS_INIT})
  target_link_libraries(cp_zmqtest PRIVATE cloud_profiler)
  target_link_libraries(cp_zmqtest PRIVATE ${ZMQ_LIBRARIES})
  target_link_libraries(cp_zmqtest PRIVATE ${Boost_LIBRARIES})

  add_custom_command(
    TARGET cp_zmqtest
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      -E copy_if_different $<TARGET_FILE:cp_zmqtest> ${CP_BINDIR}
  )
endif(CP_CONFIG_ZMQ_TEST)

if(CP_CONFIG_CPP_SINK)

  #
  # cp_cppsink
  #
  add_executable(cp_cppsink cp_cppsink.cpp)
  target_include_directories(cp_cppsink PRIVATE ${CP_SRCDIR})
  target_include_directories(cp_cppsink PUBLIC ../cxxopts/include)
  target_link_libraries(cp_cppsink PRIVATE ${CMAKE_THREAD_LIBS_INIT})
  target_link_libraries(cp_cppsink PRIVATE cloud_profiler)

  add_custom_command(
    TARGET cp_cppsink
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      -E copy_if_different $<TARGET_FILE:cp_cppsink> ${CP_BINDIR}
  )
endif(CP_CONFIG_CPP_SINK)
#
# Run tests:
#  Display summary information: make test
#  Display verbose information: ctest --verbose
#
add_test(CXX_Test cp_cpptest)
add_test(JAVA_Test java -Djava.library.path=${CP_LIBDIR} -cp ${JOPT_JAR}:${CP_JAR}:cp_jtest.jar:. cp_jtest)

add_subdirectory(jagent_test)
