qt_add_executable(test_cli_installer test_cli_installer.cpp)
target_link_libraries(test_cli_installer PRIVATE Qt6::Test)
target_compile_definitions(test_cli_installer PRIVATE
    CLI_BINARY_PATH="$<TARGET_FILE:kirawavtar-cli>"
)
add_test(NAME test_cli_installer COMMAND test_cli_installer)
