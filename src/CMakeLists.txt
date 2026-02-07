qt_add_executable(KiraWAVTar
    Main.cpp
    MainWindow.cpp MainWindow.h MainWindow.ui
    WavCombine.cpp WavCombine.h
    WavCombineDialog.cpp WavCombineDialog.h
    WavExtract.cpp WavExtract.h
    WavExtractDialog.cpp WavExtractDialog.h
    WavFormatChooserWidget.cpp WavFormatChooserWidget.h WavFormatChooserWidget.ui
    ExtractTargetSelectModel.cpp ExtractTargetSelectModel.h
    WavTarUtils.cpp WavTarUtils.h
    # Merged from KiraCommonUtils
    Translation.cpp Translation.h
    TranslationManager.cpp TranslationManager.h
    UpdateChecker.cpp UpdateChecker.h
    Filesystem.cpp Filesystem.h
    KfrHelper.h
    DirNameEditWithBrowse.cpp DirNameEditWithBrowse.h
    FileNameEditWithBrowse.cpp FileNameEditWithBrowse.h FileNameEditWithBrowse.ui
    ShowHTMLDialog.cpp ShowHTMLDialog.h ShowHTMLDialog.ui
    resources/icon.qrc
)

# Git info defines (equivalent to the qmake GIT_HASH/GIT_BRANCH/GIT_DESCRIBE)
execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git describe --tags --dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
target_compile_definitions(KiraWAVTar PRIVATE
    GIT_HASH="${GIT_HASH}"
    GIT_BRANCH="${GIT_BRANCH}"
    GIT_DESCRIBE="${GIT_DESCRIBE}"
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(KiraWAVTar PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent Qt6::Network
    kfr_io
)
