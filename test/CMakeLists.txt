include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(connection_test
    connection_test.cpp
)

target_link_libraries (connection_test httppp-static c.a)

add_test(connection_test connection_test)
