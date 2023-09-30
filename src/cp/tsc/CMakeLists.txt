# set(Boost_DEBUG 1) # Get details of package Boost localization
find_package( Boost REQUIRED COMPONENTS serialization )

if ("${Boost_LIBRARIES}" STREQUAL "")
  message("---------------------------------------------------------------")
  message("NOTE: On some distributions libraries are installed separately.")
  message("On Ubuntu, try 'apt install libboost-serialization-dev'")
  message("---------------------------------------------------------------")
endif()

find_package( Threads )

set(TSC_SLAVE_SRC_FILES
    tsc_slave.cpp
    serialization.cpp
    query_tsc/query_tsc.cpp
)

set(TSC_MASTER_SRC_FILES
    tsc_master.cpp
    serialization.cpp
)

add_executable(tsc_slave ${TSC_SLAVE_SRC_FILES})
target_include_directories(tsc_slave PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(tsc_slave PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(tsc_slave PRIVATE ../../cppzmq)
target_include_directories(tsc_slave PRIVATE ../../cxxopts/include)
target_include_directories(tsc_slave PRIVATE ${PAPI_INCLUDE_DIR})
target_link_libraries(tsc_slave PRIVATE cloud_profiler)
target_link_libraries(tsc_slave PRIVATE ${PAPI_LIBRARIES})
target_link_libraries(tsc_slave PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(tsc_slave PRIVATE ${Boost_LIBRARIES})

add_custom_command(
  TARGET tsc_slave 
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:tsc_slave> ${CP_BINDIR}
)

add_executable(tsc_master ${TSC_MASTER_SRC_FILES})
target_include_directories(tsc_master PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(tsc_master PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(tsc_master PRIVATE ../../cppzmq)
target_include_directories(tsc_master PRIVATE ../../cxxopts/include)
target_link_libraries(tsc_master PRIVATE cloud_profiler)
target_link_libraries(tsc_master PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(tsc_master PRIVATE ${Boost_LIBRARIES})

add_custom_command(
  TARGET tsc_master
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:tsc_master> ${CP_BINDIR}
)

add_executable(tsc_slave_1 tsc_slave_1.cpp) 
target_include_directories(tsc_slave_1 PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(tsc_slave_1 PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(tsc_slave_1 PRIVATE ../../cppzmq)
target_include_directories(tsc_slave_1 PRIVATE ../../cxxopts/include)
target_include_directories(tsc_slave_1 PRIVATE ${PAPI_INCLUDE_DIR})
target_link_libraries(tsc_slave_1 PRIVATE cloud_profiler)
target_link_libraries(tsc_slave_1 PRIVATE ${PAPI_LIBRARIES})
target_link_libraries(tsc_slave_1 PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(tsc_slave_1 PRIVATE ${Boost_LIBRARIES})
target_link_libraries(tsc_slave_1 PRIVATE ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(
  TARGET tsc_slave_1 
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:tsc_slave_1> ${CP_BINDIR}
)

add_executable(tsc_slave_2 tsc_slave_2.cpp) 
target_include_directories(tsc_slave_2 PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(tsc_slave_2 PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(tsc_slave_2 PRIVATE ../../cppzmq)
target_include_directories(tsc_slave_2 PRIVATE ../../cxxopts/include)
target_include_directories(tsc_slave_2 PRIVATE ${PAPI_INCLUDE_DIR})
target_link_libraries(tsc_slave_2 PRIVATE cloud_profiler)
target_link_libraries(tsc_slave_2 PRIVATE ${PAPI_LIBRARIES})
target_link_libraries(tsc_slave_2 PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(tsc_slave_2 PRIVATE ${Boost_LIBRARIES})

add_custom_command(
  TARGET tsc_slave_2 
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:tsc_slave_2> ${CP_BINDIR}
)
