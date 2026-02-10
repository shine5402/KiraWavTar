# UI Widgets
target_sources(KiraWAVTar PRIVATE
    DirNameEditWithBrowse.cpp
    DirNameEditWithBrowse.h
    FileNameEditWithBrowse.cpp
    FileNameEditWithBrowse.h
    FileNameEditWithBrowse.ui
    RemoteImageTextBrowser.cpp
    RemoteImageTextBrowser.h
    WavFormatChooserWidget.cpp
    WavFormatChooserWidget.h
    WavFormatChooserWidget.ui
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
