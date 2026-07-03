# yaml-cpp cmake stub config — used when libyaml-cpp-dev is not installed
# The runtime library is present but the -dev package is missing.
# Install libyaml-cpp-dev to replace this with the official config.
cmake_minimum_required(VERSION 3.16)

set(yaml-cpp_FOUND TRUE)
set(YAML_CPP_FOUND TRUE)

if(NOT TARGET yaml-cpp)
  add_library(yaml-cpp SHARED IMPORTED)
  find_library(YAML_CPP_LIB
    NAMES yaml-cpp
    HINTS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
    NO_DEFAULT_PATH
  )
  if(NOT YAML_CPP_LIB)
    set(YAML_CPP_LIB "/usr/lib/x86_64-linux-gnu/libyaml-cpp.so.0.8")
  endif()
  set_target_properties(yaml-cpp PROPERTIES
    IMPORTED_LOCATION "${YAML_CPP_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES ""
  )
endif()

set(YAML_CPP_LIBRARIES yaml-cpp)
set(YAML_CPP_INCLUDE_DIR "")
