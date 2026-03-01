qt_add_executable(test_sample_types
    test_sample_types.cpp
)
target_link_libraries(test_sample_types PRIVATE Qt6::Test kfr)
target_include_directories(test_sample_types PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_sample_types COMMAND test_sample_types)
