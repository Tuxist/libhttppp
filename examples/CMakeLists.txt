include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(connection connection.cpp)

target_link_libraries (connection httppp-static)

add_executable (httpcon httpcon.cpp)

target_link_libraries(httpcon httppp-static)

add_executable (httpform httpform.cpp)

target_link_libraries(httpform httppp-static)

add_executable (httpsysinfo httpsysinfo.cpp)

target_link_libraries(httpsysinfo httppp-static)

add_executable (httpcookie httpcookie.cpp)

target_link_libraries(httpcookie httppp-static)

add_executable (httpauth httpauth.cpp)

target_link_libraries(httpauth httppp-static)

add_executable (httpproxy httpproxy.cpp)

target_link_libraries(httpproxy httppp-static)
