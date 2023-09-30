# Set a default build type if none was specified
string(TOLOWER ${CMAKE_CURRENT_BINARY_DIR} LOWER_BINDIR)
set(DBG_REGEX "^.*_(debug)|(dbg)$")
if("${LOWER_BINDIR}" MATCHES "^.*(_debug)|(_dbg)$")
  set(default_build_type "Debug")
  set(reason "buildir name has DEBUG postfix '${CMAKE_MATCH_1}'")
elseif("${LOWER_BINDIR}" MATCHES "^.*(_release)|(_rel)$")
  set(default_build_type "Release")
  set(reason "buildir name has RELEASE postfix '${CMAKE_MATCH_1}'")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
  set(reason "CMAKE_SOURCE_DIR has .git directory")
else()
  set(default_build_type "Release")
  set(reason "none was specified")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}'.")
  message(STATUS "Reason: ${reason}")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
