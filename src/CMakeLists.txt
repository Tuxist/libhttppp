find_package(OpenSSL REQUIRED)

set(libhttpSrcs
  connections.cpp
  socket.cpp
  http.cpp
  httpd.cpp
  https.cpp
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   ADD_DEFINITIONS(-DWIN)
   list(APPEND libhttpSrcs "event/iocp.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   list(APPEND libhttpSrcs "event/epoll.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
   list(APPEND libhttpSrcs "event/kqueue.cpp")
ENDIF()

include_directories(
  ${OPENSSL_INCLUDE_DIR}
)

add_library(httppp        SHARED ${libhttpSrcs} )
add_library(httppp-static STATIC ${libhttpSrcs} )

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(httppp ${OPENSSL_LIBRARIES} wsock32 ws2_32)
  target_link_libraries(httppp-static ${OPENSSL_LIBRARIES} wsock32 ws2_32)
ELSE()
  target_link_libraries(httppp ${OPENSSL_LIBRARIES})
  target_link_libraries(httppp-static ${OPENSSL_LIBRARIES})    
ENDIF()

SET_TARGET_PROPERTIES(httppp PROPERTIES INTERFACE_LINK_LIBRARIES "" VERSION ${LIBV} SOVERSION 1)
SET_TARGET_PROPERTIES(httppp PROPERTIES LINK_INTERFACE_LIBRARIES "" VERSION ${LIBV} SOVERSION 1)

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

install(TARGETS httppp  DESTINATION lib EXPORT httppp-targets)
install(TARGETS httppp-static DESTINATION lib)
install(EXPORT  httppp-targets DESTINATION lib/cmake/httppp)
export(PACKAGE httppp)

install(FILES connections.h         
              httpd.h
              http.h
              httpdefinitions.h
              socket.h
              exception.h
              event.h
              https.h
              mimetypes.h
              libhttppp.cmake
              ${CMAKE_BINARY_DIR}/config.h
        DESTINATION include/httppp)
