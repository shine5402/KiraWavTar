cmake_minimum_required(VERSION 3.16)
project(KiraWavTar VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)

# --- Third-party libraries ---

# KFR (audio DSP library, custom fork with RF64 support)
set(ENABLE_DFT OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/kfr)

# --- Main application ---
add_subdirectory(src)
