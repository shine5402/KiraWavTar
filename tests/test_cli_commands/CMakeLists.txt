qt_add_executable(test_cli_combine
    test_cli_combine.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/CombineCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/FormatParser.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/ProgressReporter.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_cli_combine PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Concurrent
    kfr kfr_dsp kfr_audio
    dr_libs FLAC TBB::tbb
)
target_include_directories(test_cli_combine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_cli_combine PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_cli_combine COMMAND test_cli_combine)

qt_add_executable(test_cli_extract
    test_cli_extract.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/CombineCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/ExtractCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/FormatParser.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/ProgressReporter.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioCombine.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioExtract.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_cli_extract PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Concurrent
    kfr kfr_dsp kfr_audio
    dr_libs FLAC TBB::tbb
)
target_include_directories(test_cli_extract PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_cli_extract PRIVATE TEST_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
add_test(NAME test_cli_extract COMMAND test_cli_extract)
