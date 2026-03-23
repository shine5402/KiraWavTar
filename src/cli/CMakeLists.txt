# CLI Application
qt_add_executable(kirawavtar-cli
    main.cpp
    CliUtils.h
    CombineCommand.h CombineCommand.cpp
    ExtractCommand.h ExtractCommand.cpp
    FormatParser.h FormatParser.cpp
    ProgressReporter.h ProgressReporter.cpp
)

target_include_directories(kirawavtar-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(kirawavtar-cli PRIVATE kirawavtar_core)

set_target_properties(kirawavtar-cli PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
    INSTALL_RPATH "@executable_path/../Frameworks"
)

add_subdirectory(launcher)
