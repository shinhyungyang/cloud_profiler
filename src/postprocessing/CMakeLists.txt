#
# C++: btoa_converter
#
add_executable(btoa_converter btoa_converter.cpp)
target_include_directories(btoa_converter PUBLIC ../cxxopts/include)
target_link_libraries(btoa_converter PRIVATE cloud_profiler)
target_link_libraries(btoa_converter PRIVATE ${CMAKE_THREAD_LIBS_INIT})

find_package(SQUASH)
include_directories(${JNI_INCLUDE_DIRS} ${SQUASH_INCLUDE_DIR})
link_libraries(${SQUASH_LIBRARY})


# For convenience we copy the binaries to the bin folder:
add_custom_command(
  TARGET btoa_converter
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -E copy_if_different $<TARGET_FILE:btoa_converter> ${CP_BINDIR}
)

# btoa_file_converter.sh.in
configure_file("btoa_file_converter.sh.in" "${CP_BINDIR}/btoa_file_converter.sh")
