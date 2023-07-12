project(libhttppp)
cmake_minimum_required(VERSION 3.0)
enable_testing ()

set(LIBV "1.0.0")
set(Upstream_VERSION 1.0.0)
set (BLOCKSIZE 16384 CACHE STRING "Block size from Network Packages")

include(CheckIncludeFileCXX)

find_package(libnetplus REQUIRED)
find_package(libcryptplus REQUIRED)

SET(CMAKE_CXX_FLAGS "-fPIC -Wall ")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  ${CMAKE_SOURCE_DIR}/cmake/)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)
