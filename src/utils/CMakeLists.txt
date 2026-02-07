# Utilities
target_sources(KiraWAVTar PRIVATE
    Filesystem.cpp
    Filesystem.h
    KfrHelper.h
    TranslationManager.cpp
    TranslationManager.h
    UpdateChecker.cpp
    UpdateChecker.h
    Utils.cpp
    Utils.h
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
