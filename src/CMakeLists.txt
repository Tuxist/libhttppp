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

add_library(http++        SHARED ${libhttpSrcs} )
add_library(http++-static STATIC ${libhttpSrcs} )

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(http++ ${OPENSSL_LIBRARIES})
  target_link_libraries(http++-static ${OPENSSL_LIBRARIES})
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
              event.h
              session.h
              https.h
              mimetypes.h
              libhttp++.cmake
              ${CMAKE_BINARY_DIR}/config.h
        DESTINATION include/http++)
