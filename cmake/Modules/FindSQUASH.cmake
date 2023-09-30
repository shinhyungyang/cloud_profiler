##{ find SQUASH library
set(SQUASH_PREFIX "" CACHE PATH "The path to the prefix of a SQUASH installation")

find_library(SQUASH_LIBRARY NAMES squash0.8
    HINTS ${SQUASH_PREFIX} ENV LD_LIBRARY_PATH
    #PATHS /opt/squash/0.7/lib /usr/local
    PATHS /opt/squash/0.8/lib /usr/local
    PATH_SUFFIXES lib)
find_path(SQUASH_INCLUDE_DIR squash.h squash
    HINTS ${SQUASH_PREFIX}
    #PATHS /opt/squash/0.7/include/squash-0.7 /usr/local
    PATHS /opt/squash/0.8/include/squash-0.8 /usr/local
    PATH_SUFFIXES squash-0.8)

message(STATUS "${SQUASH_LIBRARY}, ${SQUASH_INCLUDE_DIR}")

if(SQUASH_INCLUDE_DIR AND SQUASH_LIBRARY)
  message(STATUS "Found SQUASH: ${SQUASH_LIBRARY}, ${SQUASH_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "Could not find SQUASH")
endif()
