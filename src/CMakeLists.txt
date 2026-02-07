add_subdirectory(3rdparty)

qt_add_executable(KiraWAVTar
    main.cpp
    resources.qrc
)

# Add module subdirectories
# Each module manages its own source files via target_sources()
add_subdirectory(ui)
add_subdirectory(worker)
add_subdirectory(utils)

# Root include directory for cross-module includes
target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(KiraWAVTar PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent Qt6::Network
    kfr kfr_dsp
    dr_libs
)

set_target_properties(KiraWAVTar PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# Translations - auto-generates .ts files and embeds .qm in resources at :/i18n
qt_add_translations(KiraWAVTar
    TS_FILE_BASE KiraWavTar
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/translations
)
