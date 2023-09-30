####################
# Program Properties
set(EXECUTABLE ntplogd)

find_package( Boost REQUIRED COMPONENTS chrono )

###########################################
# ntplogd
###########################################
set(NTPLOGD_SRC_FILES
  core.cpp
  exec.cpp
) 
add_executable(${EXECUTABLE} ${NTPLOGD_SRC_FILES})
target_include_directories(${EXECUTABLE} PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(${EXECUTABLE} PRIVATE ../../cxxopts/include)
target_link_libraries(${EXECUTABLE} PRIVATE cloud_profiler)

add_custom_command(
  TARGET ${EXECUTABLE}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:${EXECUTABLE}> ${CP_BINDIR}
)

###########################################
# installation path
###########################################
install(TARGETS ${EXECUTABLE} DESTINATION bin)
