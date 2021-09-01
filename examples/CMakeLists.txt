find_package(htmlpp COMPONENTS)

option(BUILD_EXAMPLES "Create and install the HTML examples (requires Doxygen)" [TRUE])

if(BUILD_EXAMPLES)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${htmlpp_INCLUDE_DIRs}
    )
    add_executable (httpcon httpcon.cpp)
    target_link_libraries(httpcon httppp-static ${htmlpp_LIBRARIES})
endif()

if(BUILD_EXAMPLES AND ${htmlpp_FOUND})
    add_executable (httpform httpform.cpp)
    target_link_libraries(httpform httppp-static ${htmlpp_LIBRARIES} )
    add_executable (httpsysinfo httpsysinfo.cpp)
    target_link_libraries(httpsysinfo httppp-static ${htmlpp_LIBRARIES})
    add_executable (httpcookie httpcookie.cpp)
    target_link_libraries(httpcookie httppp-static ${htmlpp_LIBRARIES})
    add_executable (httpauth httpauth.cpp)
    target_link_libraries(httpauth httppp-static ${htmlpp_LIBRARIES}) 
    add_executable (httpproxy httpproxy.cpp)
    target_link_libraries(httpproxy httppp-static ${htmlpp_LIBRARIES})
endif()
