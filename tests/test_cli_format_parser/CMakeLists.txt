qt_add_executable(test_format_parser
    test_format_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/FormatParser.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
)
target_link_libraries(test_format_parser PRIVATE
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
target_include_directories(test_format_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_format_parser COMMAND test_format_parser)
