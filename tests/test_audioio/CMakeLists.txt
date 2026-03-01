qt_add_executable(test_format_detection
    test_format_detection.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
)
target_link_libraries(test_format_detection PRIVATE
    Qt6::Test
    Qt6::Widgets
    kfr
    kfr_dsp
    kfr_audio
    dr_libs
    FLAC
)
target_include_directories(test_format_detection PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_format_detection PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_format_detection COMMAND test_format_detection)

qt_add_executable(test_roundtrip
    test_roundtrip.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
)
target_link_libraries(test_roundtrip PRIVATE
    Qt6::Test
    Qt6::Widgets
    kfr
    kfr_dsp
    kfr_audio
    dr_libs
    FLAC
)
target_include_directories(test_roundtrip PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_roundtrip PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_roundtrip COMMAND test_roundtrip)
