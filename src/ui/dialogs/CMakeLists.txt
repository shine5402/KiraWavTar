# UI Dialogs
target_sources(KiraWAVTar PRIVATE
    CommonHtmlDialog.cpp
    CommonHtmlDialog.h
    CommonHtmlDialog.ui
    WavCombineDialog.cpp
    WavCombineDialog.h
    WavExtractDialog.cpp
    WavExtractDialog.h
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
