project(libhttppp)
cmake_minimum_required(VERSION 3.26)
enable_testing ()

set(LIBV "1.0.0")
set(Upstream_VERSION 1.0.0)
set (BLOCKSIZE 1500 CACHE STRING "Block size from Network Packages")
set (DEFAULT_UPLOADSIZE 4e+6 CACHE STRING "Block size from Network Packages")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(libnetplus REQUIRED)
find_package(libcmdplus REQUIRED)

find_package(htmlpp)

include(CheckIncludeFileCXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  ${CMAKE_SOURCE_DIR}/cmake/)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
    ${NETPLUS_INCLUDE_DIR}
	${CMDPLUS_INCLUDE_DIR}
)

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)
