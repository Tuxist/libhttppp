find_library(OpenSSL REQUIRED)

set(libhttpSrcs
  connections.cpp
  socket.cpp
  http.cpp
  httpd.cpp
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   ADD_DEFINITIONS(-DWIN)
   list(APPEND libhttpSrcs "queues/iocp.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   list(APPEND libhttpSrcs "queues/epoll.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
   list(APPEND libhttpSrcs "queues/kqueue.cpp")
ENDIF()

include_directories(
  ${OPENSSL_INCLUDE_DIR}
) 

add_library(http++        SHARED ${libhttpSrcs} )
add_library(http++-static STATIC ${libhttpSrcs} )

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(http++ ${OPENSSL_LIBRARIES} wsock32 ws2_32)
  target_link_libraries(http++-static ${OPENSSL_LIBRARIES} wsock32 ws2_32)
ELSE()
  target_link_libraries(http++ ${OPENSSL_LIBRARIES})
  target_link_libraries(http++-static ${OPENSSL_LIBRARIES})    
ENDIF()

SET_TARGET_PROPERTIES(http++ PROPERTIES LINK_INTERFACE_LIBRARIES "" VERSION ${LIBV} SOVERSION 1)
SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

install(TARGETS http++  DESTINATION lib EXPORT http++-targets)
install(TARGETS http++-static DESTINATION lib)
install(EXPORT  http++-targets DESTINATION lib/cmake/http++)

install(FILES connections.h           
              httpd.h                 
              http.h        
              httpdefinitions.h
              socket.h                
              exception.h
              queue.h
              libhttp++.cmake
              ${CMAKE_BINARY_DIR}/config.h
        DESTINATION include/http++)