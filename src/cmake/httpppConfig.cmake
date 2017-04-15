include(CMakeFindDependencyMacro)
find_dependency(OpenSSL 1.0.0)

include("${CMAKE_CURRENT_LIST_DIR}/httpppTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/httpppMacros.cmake")

set(_supported_components Plot Table)

foreach(_comp ${httppp_FIND_COMPONENTS})
  if (NOT ";${_supported_components};" MATCHES _comp)
    set(httppp_FOUND False)
    set(httppp_NOT_FOUND_MESSAGE "Unsupported component: ${_comp}")
  endif()
  include("${CMAKE_CURRENT_LIST_DIR}/httppp${_comp}Targets.cmake")
endforeach()
