add_subdirectory(3rdparty)

# --- Shared core library (worker + utils, no Qt Widgets) ---
add_library(kirawavtar_core OBJECT)

target_include_directories(kirawavtar_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(kirawavtar_core PUBLIC
    Qt6::Core Qt6::Concurrent
    kfr kfr_dsp kfr_audio
    dr_libs
    FLAC
    TBB::tbb
)

# Add core source modules
add_subdirectory(worker)
add_subdirectory(utils)

# --- GUI Application ---
qt_add_executable(KiraWAVTar
    main.cpp
    resources.qrc
)

add_subdirectory(ui)

# GUI-only utility sources (depend on Qt Widgets / Network)
target_sources(KiraWAVTar PRIVATE
    utils/GuiUtils.h
    utils/TranslationManager.cpp
    utils/TranslationManager.h
    utils/UpdateChecker.cpp
    utils/UpdateChecker.h
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(KiraWAVTar PRIVATE
    kirawavtar_core
    Qt6::Gui Qt6::Widgets Qt6::Network
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

# --- CLI Application ---
add_subdirectory(cli)

# --- Install & Deploy ---
option(KIRAWAVTAR_INSTALL "Enable install and deploy rules" ON)

if(KIRAWAVTAR_INSTALL)
    install(TARGETS KiraWAVTar
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(TARGETS kirawavtar-cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # oneTBB is built as a shared library; deploy it alongside the executable.
    if(APPLE)
        # macOS: place the dylib inside the app bundle's Frameworks directory
        # so that @rpath resolution finds it at runtime.
        install(TARGETS tbb
            LIBRARY DESTINATION KiraWAVTar.app/Contents/Frameworks
        )
    else()
        install(TARGETS tbb
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll (Windows)
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}    # .so (Linux)
        )
    endif()

    option(KIRAWAVTAR_DEPLOY_QT "Run Qt deploy script to bundle Qt runtime dependencies" ON)
    if(KIRAWAVTAR_DEPLOY_QT)
        qt_generate_deploy_app_script(
            TARGET KiraWAVTar
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
        )
        install(SCRIPT ${deploy_script})
    endif()

    # Linux desktop integration
    if(UNIX AND NOT APPLE)
        include(GNUInstallDirs)
        install(FILES "${CMAKE_SOURCE_DIR}/data/top.shine5402.KiraWavTar.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
        )
        install(FILES "${CMAKE_SOURCE_DIR}/data/top.shine5402.KiraWavTar.metainfo.xml"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
        )
        install(FILES "${CMAKE_SOURCE_DIR}/assets/icon-square.svg"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
            RENAME "top.shine5402.KiraWavTar.svg"
        )
        install(FILES "${CMAKE_SOURCE_DIR}/assets/icon256.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
            RENAME "top.shine5402.KiraWavTar.png"
        )
    endif()
endif()
