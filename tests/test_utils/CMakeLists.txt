enable_testing()

qt_add_executable(test_timecode
    test_timecode.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
)
target_link_libraries(test_timecode PRIVATE Qt6::Test Qt6::Widgets kfr)
target_include_directories(test_timecode PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_timecode COMMAND test_timecode)

qt_add_executable(test_filenames
    test_filenames.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
)
target_link_libraries(test_filenames PRIVATE Qt6::Test Qt6::Widgets kfr)
target_include_directories(test_filenames PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_filenames COMMAND test_filenames)
