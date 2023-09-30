####################
# Program Properties
set(EXECUTABLE armfreq)

find_package(Threads REQUIRED)
###########################################
# armfreq
###########################################
set(ARMFREQ_SRC_FILES
  barrier.cpp
  clock_test.cpp
) 
add_executable(${EXECUTABLE} ${ARMFREQ_SRC_FILES})
target_include_directories(${EXECUTABLE} PRIVATE ../../cxxopts/include)
target_link_libraries(${EXECUTABLE} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
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
