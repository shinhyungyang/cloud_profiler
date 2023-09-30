if(NOT CP_CONFIG_SERVER)
  add_library(net_conf STATIC net_conf_empty.cpp)
  return() #early exit
endif(NOT CP_CONFIG_SERVER)

#find_package( Boost 1.66 REQUIRED )

###########################################
# net_conf
###########################################
set(NET_CONF_SRC_FILES
    net_conf.cpp
    parser.cpp
    fox/fox_config_server.cpp
)

add_library(net_conf SHARED ${NET_CONF_SRC_FILES})
target_include_directories(net_conf PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(net_conf PUBLIC ${ZMQ_INCLUDE_DIR})
target_include_directories(net_conf PUBLIC ../../cppzmq)
target_link_libraries(net_conf PUBLIC ${ZMQ_LIBRARIES})
target_link_libraries(net_conf PUBLIC ${Boost_LIBRARIES})

###########################################
# config_server
###########################################
set(CONFIG_SERVER_SRC_FILES
    config_server.cpp
)

add_executable(config_server ${CONFIG_SERVER_SRC_FILES})
target_include_directories(config_server PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(config_server PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(config_server PRIVATE ../../cppzmq)
target_include_directories(config_server PRIVATE ../../cxxopts/include)
target_link_libraries(config_server PRIVATE cloud_profiler)
target_link_libraries(config_server PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(config_server PRIVATE ${Boost_LIBRARIES})

add_custom_command(
  TARGET config_server 
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:config_server> ${CP_BINDIR}
)

###########################################
# config_client
###########################################
set(CONFIG_CLIENT_SRC_FILES
    config_client.cpp
)

add_executable(config_client ${CONFIG_CLIENT_SRC_FILES})
target_include_directories(config_client PRIVATE ${ZMQ_INCLUDE_DIR})
target_include_directories(config_client PRIVATE ../../cppzmq)
target_include_directories(config_client PRIVATE ../../cxxopts/include)
target_link_libraries(config_client PRIVATE cloud_profiler)
target_link_libraries(config_client PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(config_client PRIVATE ${Boost_LIBRARIES})

add_custom_command(
  TARGET config_client
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:config_client> ${CP_BINDIR}
)

###########################################
# installation path
###########################################
install(TARGETS net_conf DESTINATION lib)
install(TARGETS config_server DESTINATION bin)
install(TARGETS config_client DESTINATION bin)
