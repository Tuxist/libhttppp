project(libhttppp)
cmake_minimum_required(VERSION 2.6)
enable_testing ()

set(LIBV "1.0.0")
set(Upstream_VERSION 1.0.0)
set (BLOCKSIZE "16384" CACHE STRING "Block size from Network Packages")

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
  if( CMAKE_CXX_COMPILER_ID MATCHES GNU )
     SET(CMAKE_CXX_FLAGS "-g -Wall")
  endif( CMAKE_CXX_COMPILER_ID MATCHES GNU )
  if(CMAKE_CXX_COMPILER_ID MATCHES ".*clang")
    SET(CMAKE_CXX_FLAGS "-g -Wthread-safety -Wall")
  endif( CMAKE_CXX_COMPILER_ID MATCHES ".*clang" )
ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASERELEASE)

if( CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  SET(CMAKE_CXX_FLAGS "-D_SCL_SECURE_NO_WARNINGS  /EHsc")	
endif( CMAKE_CXX_COMPILER_ID MATCHES MSVC)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  ${CMAKE_SOURCE_DIR}/cmake/)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(EVENT "IOCP" CACHE STRING "IOCP")
    SET(EVENT_HEADER "iocp/iocp.h" CACHE STRING "iocp/iocp.h")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(EVENT "EPOLL" CACHE STRING "EPOLL")
    SET(EVENT_HEADER "epoll/epoll.h" CACHE STRING "epoll/epoll.h")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR 
       ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(EVENT "KQUEUE" CACHE STRING "KQUEUE")
    SET(EVENT_HEADER "kqueue/kqueue.h" CACHE STRING "kqueue/kqueue.h")
ENDIF()

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(event.h.in ${CMAKE_CURRENT_BINARY_DIR}/event.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)
