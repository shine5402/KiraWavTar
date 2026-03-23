if(WIN32)
    add_executable(kirawavtar-cli-launcher main.c)

    set_target_properties(kirawavtar-cli-launcher PROPERTIES
        OUTPUT_NAME "kirawavtar-cli-launcher"
        WIN32_EXECUTABLE OFF
    )

    # Static CRT so the launcher has zero DLL dependencies
    set_target_properties(kirawavtar-cli-launcher PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Version info resource
    configure_file(launcher.rc.in "${CMAKE_CURRENT_BINARY_DIR}/launcher.rc" @ONLY)
    target_sources(kirawavtar-cli-launcher PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/launcher.rc")
endif()
