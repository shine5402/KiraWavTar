cmake_minimum_required(VERSION 3.16)
project(KiraWavTar VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)

qt_standard_project_setup()

# --- Third-party libraries ---

# KFR (audio DSP library)
set(ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DSP ON CACHE BOOL "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/kfr)

# dr_libs
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dr_libs)

# --- Main application ---
add_subdirectory(src)
