#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Upstream::httppp" for configuration ""
set_property(TARGET Upstream::httppp APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(Upstream::httppp PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "/usr/lib/x86_64-linux-gnu/libssl.so;/usr/lib/x86_64-linux-gnu/libcrypto.so;pthread"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libhttppp.so.1.0.0"
  IMPORTED_SONAME_NOCONFIG "libhttppp.so.1.0.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS Upstream::httppp )
list(APPEND _IMPORT_CHECK_FILES_FOR_Upstream::httppp "${_IMPORT_PREFIX}/lib/libhttppp.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
