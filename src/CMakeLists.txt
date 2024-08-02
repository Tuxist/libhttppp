set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

include(GenerateExportHeader)

set(libhttpSrcs
  exception.cpp
  http.cpp
  httpd.cpp
)

include_directories(
    ${MbedTLS_INCLUDE_DIRS}
    ${systempp_INCLUDE_DIR}
)

add_library(httppp        SHARED ${libhttpSrcs} )

target_compile_definitions(httppp PUBLIC -DSHARED)

add_library(httppp-static STATIC ${libhttpSrcs} )

generate_export_header(httppp)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(httppp MbedTLS::mbedcrypto netplus::netplus cmdplus::cmdplus)
  target_link_libraries(httppp-static MbedTLS::mbedcrypto netplus::netplus cmdplus::cmdplus)
else()
  target_link_libraries(httppp MbedTLS::mbedcrypto netplus::netplus cmdplus::cmdplus pthread)
  target_link_libraries(httppp-static MbedTLS::mbedcrypto netplus::netplus cmdplus::cmdplus pthread)
endif()

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

install(FILES httpd.h
              http.h
              httpdefinitions.h
              exception.h
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
  NAMESPACE httppp::
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
    httppp::
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
