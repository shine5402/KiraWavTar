qt_add_executable(test_combine
    test_combine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_combine PRIVATE
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
target_include_directories(test_combine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_combine PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_combine COMMAND test_combine)

qt_add_executable(test_extract
    test_extract.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioExtract.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_extract PRIVATE
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
target_include_directories(test_extract PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_extract PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_extract COMMAND test_extract)

qt_add_executable(test_roundtrip_full
    test_roundtrip_full.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioExtract.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_roundtrip_full PRIVATE
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
target_include_directories(test_roundtrip_full PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_roundtrip_full PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_roundtrip_full COMMAND test_roundtrip_full)
