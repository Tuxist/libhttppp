 get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
 include(${SELF_DIR}/httppp-targets.cmake)
 get_filename_component(httppp_INCLUDE_DIRS "${SELF_DIR}/../../../include/libhttppp" ABSOLUTE)
