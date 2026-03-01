cmake_minimum_required(VERSION 3.16)
project(KiraWavTar VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network LinguistTools)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES zh_CN ja_JP zh_TW
)

# --- Main application ---
add_subdirectory(src)

# --- Testing ---
# Use KIRAWAVTAR_BUILD_TESTING instead of BUILD_TESTING to avoid collision
# with libFLAC which also uses BUILD_TESTING (we disable it in 3rdparty/CMakeLists.txt)
option(KIRAWAVTAR_BUILD_TESTING "Build KiraWavTar tests" ON)
if(KIRAWAVTAR_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
