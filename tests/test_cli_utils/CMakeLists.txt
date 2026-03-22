qt_add_executable(test_cli_utils
    test_cli_utils.cpp
)
target_link_libraries(test_cli_utils PRIVATE Qt6::Test)
target_include_directories(test_cli_utils PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_cli_utils COMMAND test_cli_utils)
