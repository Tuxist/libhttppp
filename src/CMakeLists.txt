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

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

include(GenerateExportHeader)

generate_export_header(httppp)

set_property(TARGET httppp PROPERTY VERSION ${Upstream_VERSION})
set_property(TARGET httppp PROPERTY SOVERSION ${LIBV})
set_property(TARGET httppp PROPERTY INTERFACE_httppp_MAJOR_VERSION ${LIBV})
set_property(TARGET httppp APPEND PROPERTY COMPATIBLE_INTERFACE_STRING httppp_MAJOR_VERSION)

install(TARGETS httppp EXPORT httpppTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(FILES connections.h         
              httpd.h
              http.h
              httpdefinitions.h
              socket.h
              exception.h
              event.h
              https.h
              mimetypes.h
              "${CMAKE_BINARY_DIR}/config.h"
              "${CMAKE_CURRENT_BINARY_DIR}/httppp_export.h"
        DESTINATION include/httppp
        COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppConfigVersion.cmake"
  VERSION ${Upstream_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT httpppTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppTargets.cmake"
  NAMESPACE Upstream::
)
configure_file(cmake/httpppConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/httppp)
install(EXPORT httpppTargets
  FILE
    httpppTargets.cmake
  NAMESPACE
    Upstream::
  DESTINATION
    ${ConfigPackageLocation}
)

install(
  FILES
    cmake/httpppConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)
