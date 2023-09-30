
###########################################
# fox_end_client
###########################################
set(CONFIG_CLIENT_FOX_SRC_FILES
  fox_end_client.cpp
) 
add_executable(fox_end_client ${CONFIG_CLIENT_FOX_SRC_FILES})
target_include_directories(fox_end_client PRIVATE ../../cxxopts/include)
target_link_libraries(fox_end_client PRIVATE cloud_profiler)

add_custom_command(
  TARGET fox_end_client
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:fox_end_client> ${CP_BINDIR}
)

###########################################
# fox_start_client
###########################################
set(CONFIG_CLIENT_FOX_SRC_FILES
  fox_start_client.cpp
) 
add_executable(fox_start_client ${CONFIG_CLIENT_FOX_SRC_FILES})
target_include_directories(fox_start_client PRIVATE ../../cxxopts/include)
target_link_libraries(fox_start_client PRIVATE cloud_profiler)

add_custom_command(
  TARGET fox_start_client
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:fox_start_client> ${CP_BINDIR}
)

###########################################
# installation path
###########################################
install(TARGETS fox_end_client DESTINATION bin)
install(TARGETS fox_start_client DESTINATION bin)
