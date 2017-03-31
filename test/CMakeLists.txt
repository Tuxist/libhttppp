include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable (httpcon httpcon.cpp)

target_link_libraries(httpcon http++-static)

add_executable (httpform httpform.cpp)

target_link_libraries(httpform http++-static)

add_executable (httpsysinfo httpsysinfo.cpp)

target_link_libraries(httpsysinfo http++-static)
