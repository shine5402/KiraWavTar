# Utilities — core (shared by GUI and CLI)
target_sources(kirawavtar_core PRIVATE
    Filesystem.cpp
    Filesystem.h
    KfrHelper.h
    Utils.cpp
    Utils.h
)

target_include_directories(kirawavtar_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
