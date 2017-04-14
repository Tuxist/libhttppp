include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable (httpcon httpcon.cpp)

target_link_libraries(httpcon httppp-static)

add_executable (httpform httpform.cpp)

target_link_libraries(httpform httppp-static)

add_executable (httpsysinfo httpsysinfo.cpp)

target_link_libraries(httpsysinfo httppp-static)
