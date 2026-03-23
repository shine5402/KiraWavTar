if(WIN32)
    qt_add_executable(test_cli_launcher test_cli_launcher.cpp)
    target_link_libraries(test_cli_launcher PRIVATE Qt6::Test)
    target_compile_definitions(test_cli_launcher PRIVATE
        CLI_BINARY_PATH="$<TARGET_FILE:kirawavtar-cli>"
        LAUNCHER_BINARY_PATH="$<TARGET_FILE:kirawavtar-cli-launcher>"
    )
    add_test(NAME test_cli_launcher COMMAND test_cli_launcher)
endif()
