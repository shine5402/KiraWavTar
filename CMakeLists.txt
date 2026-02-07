cmake_minimum_required(VERSION 3.16)
project(KiraWavTar VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)

qt_standard_project_setup()

# --- Main application ---
add_subdirectory(src)
