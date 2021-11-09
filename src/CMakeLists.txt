set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

include(GenerateExportHeader)

set(libhttpSrcs
  threadpool.cpp
  exception.cpp
  connections.cpp
  http.cpp
  httpd.cpp
  https.cpp
  base64.cpp
  utils.cpp
  event.cpp
)

IF(${EVENT} MATCHES "SELECT")
    list(APPEND libhttpSrcs "event/select/select.cpp")
ELSEIF(${EVENT} MATCHES "IOCP")
    list(APPEND libhttpSrcs "event/iocp/iocp.cpp")
ELSEIF(${EVENT} MATCHES "EPOLL")
    list(APPEND libhttpSrcs "event/epoll/epoll.cpp")
ELSEIF(${EVENT} MATCHES "KQUEUE")
    list(APPEND libhttpSrcs "event/kqueue/kqueue.cpp")
ENDIF()

include_directories(
  ${OPENSSL_INCLUDE_DIR}
)

add_library(httppp        SHARED ${libhttpSrcs} )
add_library(httppp-static STATIC ${libhttpSrcs} )

generate_export_header(httppp)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(httppp ${OPENSSL_LIBRARIES} wsock32 ws2_32 systempp)
  target_link_libraries(httppp-static ${OPENSSL_LIBRARIES} wsock32 ws2_32 systempp)
ELSE()
  target_link_libraries(httppp ${OPENSSL_LIBRARIES} systempp)
  target_link_libraries(httppp-static ${OPENSSL_LIBRARIES} systempp)    
ENDIF()

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

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

install(TARGETS httppp-static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(FILES connections.h
              threadpool.h
              httpd.h
              http.h
              httpdefinitions.h
              exception.h
              eventapi.h
              https.h
              mimetypes.h
              utils.h
              "${CMAKE_BINARY_DIR}/config.h"
              "${CMAKE_BINARY_DIR}/event.h"
              "${CMAKE_CURRENT_BINARY_DIR}/httppp_export.h"
        DESTINATION include/httppp
        COMPONENT Devel
)

#os dependencies
install(FILES os/os.h
        DESTINATION include/httppp/os
        COMPONENT Devel
)


IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES event/epoll/epoll.h 
            DESTINATION include/httppp/event/epoll
            COMPONENT Devel
    )
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR 
       ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install(FILES event/kqueue/kqueue.h
            DESTINATION include/httppp/event/kqueue
            COMPONENT Devel
    )
ENDIF()

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
  @ONLY
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
    "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/httppp/httpppConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)
