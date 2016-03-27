 get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
 include(${SELF_DIR}/http++-targets.cmake)
 get_filename_component(http++_INCLUDE_DIRS "${SELF_DIR}/../../../include/libhttp++" ABSOLUTE)