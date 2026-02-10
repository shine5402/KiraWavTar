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
    FLAC
)

set_target_properties(KiraWAVTar PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(WIN32)
    configure_file(app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(KiraWAVTar PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

if(APPLE)
    set(app_icon "${CMAKE_SOURCE_DIR}/assets/KiraWavTar.icns")
    target_sources(KiraWAVTar PRIVATE "${app_icon}")
    set_source_files_properties("${app_icon}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_target_properties(KiraWAVTar PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "KiraWavTar.icns"
        MACOSX_BUNDLE_GUI_IDENTIFIER "top.shine5402.KiraWavTar"
    )
endif()

# --- Separate Debug Info (macOS, RelWithDebInfo) ---
if(APPLE AND OSX_SEPARATE_DEBUG_INFO AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_custom_command(TARGET KiraWAVTar POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:KiraWAVTar>.dSYM/Contents/Resources/DWARF"
        COMMAND /usr/bin/dsymutil --flat
            -o "$<TARGET_BUNDLE_DIR:KiraWAVTar>.dSYM/Contents/Resources/DWARF/KiraWAVTar"
            "$<TARGET_FILE:KiraWAVTar>"
        COMMAND /usr/bin/strip -S "$<TARGET_FILE:KiraWAVTar>"
        COMMENT "Extracting dSYM and stripping KiraWAVTar"
        VERBATIM
    )
endif()

# Translations - auto-generates .ts files and embeds .qm in resources at :/i18n
qt_add_translations(KiraWAVTar
    TS_FILE_BASE KiraWavTar
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/translations
    MERGE_QT_TRANSLATIONS
)

# --- Install & Deploy ---
option(KIRAWAVTAR_INSTALL "Enable install and deploy rules" ON)

if(KIRAWAVTAR_INSTALL)
    install(TARGETS KiraWAVTar
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    qt_generate_deploy_app_script(
        TARGET KiraWAVTar
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
