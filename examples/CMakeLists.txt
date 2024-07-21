find_package(htmlpp COMPONENTS)

option(BUILD_EXAMPLES "Create and install the HTML examples (requires Doxygen)" TRUE)

if(BUILD_EXAMPLES AND ${htmlpp_FOUND})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${systempp_INCLUDE_DIRS}
        ${htmlpp_INCLUDE_DIRS}
    )
    add_executable (httpcon httpcon.cpp)
    target_link_libraries(httpcon httppp-static htmlpp::htmlpp-static)
    add_executable (httpform httpform.cpp)
    target_link_libraries(httpform httppp-static htmlpp::htmlpp-static)
    add_executable (httpsysinfo httpsysinfo.cpp)
    target_link_libraries(httpsysinfo httppp-static htmlpp::htmlpp-static)
    add_executable (httpcookie httpcookie.cpp)
    target_link_libraries(httpcookie httppp-static htmlpp::htmlpp-static)
    add_executable (httpauth httpauth.cpp)
    target_link_libraries(httpauth httppp-static htmlpp::htmlpp-static)
    add_executable (httpproxy httpproxy.cpp)
    target_link_libraries(httpproxy httppp-static htmlpp::htmlpp-static)
    add_executable (httphello httphello.cpp)
    target_link_libraries(httphello httppp-static htmlpp::htmlpp-static)
endif()

add_executable (httpclient httpclient.cpp)
target_link_libraries(httpclient httppp-static)
