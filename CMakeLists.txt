project(libhttppp)
cmake_minimum_required(VERSION 2.6)
enable_testing ()

set(LIBV "1.0.0")
set(Upstream_VERSION 1.0.0)
set (BLOCKSIZE 16384 CACHE STRING "Block size from Network Packages")

find_package(systempp REQUIRED)


if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  SET(CMAKE_CXX_FLAGS "-D_SCL_SECURE_NO_WARNINGS  /EHsc")
else()
  SET(CMAKE_CXX_FLAGS "") #goal -nolibc
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  ${CMAKE_SOURCE_DIR}/cmake/)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(EVENT "EPOLL" CACHE STRING "EPOLL")
    SET(EVENT_HEADER "epoll/epoll.h" CACHE STRING "epoll/epoll.h")
#ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    #SET(EVENT "IOCP" CACHE STRING "IOCP")
    #SET(EVENT_HEADER "iocp/iocp.h" CACHE STRING "iocp/iocp.h")
#ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR 
       #${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #SET(EVENT "KQUEUE" CACHE STRING "KQUEUE")
    #SET(EVENT_HEADER "kqueue/kqueue.h" CACHE STRING "kqueue/kqueue.h")
ELSE()
    SET(EVENT "SELECT" CACHE STRING "SELECT")
    SET(EVENT_HEADER "select/select.h" CACHE STRING "select/select.h")
ENDIF()

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(event.h.in ${CMAKE_CURRENT_BINARY_DIR}/event.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)
