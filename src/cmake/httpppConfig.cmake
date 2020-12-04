include("${CMAKE_CURRENT_LIST_DIR}/httpppTargets.cmake")

get_filename_component(httppp_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
if(EXISTS "${httppp_CMAKE_DIR}/CMakeCache.txt")
  # In build tree
  include("${httppp_CMAKE_DIR}/httpppBuildTreeSettings.cmake")
else()
  set(httppp_INCLUDE_DIRS "@CMAKE_INSTALL_PREFIX@/include/httppp")
endif()

set(httppp_LIBRARIES httppp)
set(httppp_INCLUDE_DIR "httppp")

