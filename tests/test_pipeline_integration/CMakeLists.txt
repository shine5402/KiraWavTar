qt_add_executable(test_combine_config
    test_combine_config.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_combine_config PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Concurrent
    kfr
    kfr_dsp
    kfr_audio
    dr_libs
    FLAC
    TBB::tbb
)
target_include_directories(test_combine_config PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_combine_config PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_combine_config COMMAND test_combine_config)

qt_add_executable(test_extract_config
    test_extract_config.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioExtract.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_extract_config PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Concurrent
    kfr
    kfr_dsp
    kfr_audio
    dr_libs
    FLAC
    TBB::tbb
)
target_include_directories(test_extract_config PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_extract_config PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_extract_config COMMAND test_extract_config)
