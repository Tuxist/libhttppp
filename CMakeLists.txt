project(libhttp++)
cmake_minimum_required(VERSION 2.6)
enable_testing ()

#if(CMAKE_CXX_COMPILER MATCHES "gcc")
  SET(CMAKE_CXX_FLAGS "-O0 -g -Wall -std=c++11")
#endif(CMAKE_CXX_COMPILER MATCHES "gcc")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  ${CMAKE_SOURCE_DIR}/cmake/)

set(LIBV "1.0.0")

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  list(APPEND ${CMAKE_CXX_FLAGS} "-msse2")
  add_definitions( -Dx86_64)
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(test)