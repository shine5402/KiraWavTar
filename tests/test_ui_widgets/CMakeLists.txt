qt_add_executable(test_wav_format_chooser
    test_wav_format_chooser.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/WavFormatChooserWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs/CommonHtmlDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/RemoteImageTextBrowser.cpp
    ${CMAKE_SOURCE_DIR}/src/worker/AudioIO.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Filesystem.cpp
)
target_link_libraries(test_wav_format_chooser PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    kfr
    kfr_dsp
    kfr_audio
    dr_libs
    FLAC
    TBB::tbb
)
target_include_directories(test_wav_format_chooser PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs
)
add_test(NAME test_wav_format_chooser COMMAND test_wav_format_chooser)
