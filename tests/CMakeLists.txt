find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

set(TEST_FIXTURES_DIR ${CMAKE_CURRENT_BINARY_DIR}/fixtures)

function(copy_fixture FILENAME)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${FILENAME}
        ${TEST_FIXTURES_DIR}/${FILENAME}
        COPYONLY
    )
endfunction()

copy_fixture(LJ001-0001.wav)
copy_fixture(LJ001-0005.wav)
copy_fixture(LJ001-0010.wav)
copy_fixture(LJ001-0015.wav)
copy_fixture(LJ001-0020.wav)
copy_fixture(LJ001-0025.wav)
copy_fixture(LJ001-0030.wav)
copy_fixture(LJ001-0035.wav)
copy_fixture(LJ001-0040.wav)
copy_fixture(LJ001-0045.wav)

add_subdirectory(test_utils)
add_subdirectory(test_kfrhelper)
add_subdirectory(test_audioio)
add_subdirectory(test_pipeline)
add_subdirectory(test_ui_widgets)
add_subdirectory(test_pipeline_integration)
add_subdirectory(test_cli_format_parser)
add_subdirectory(test_cli_utils)
add_subdirectory(test_cli_commands)

option(KIRAWAVTAR_TEST_CLI_INSTALLER "Build CLI installer integration tests (modifies system state)" OFF)
if(KIRAWAVTAR_TEST_CLI_INSTALLER)
    add_subdirectory(test_cli_installer)
endif()
