cmake_minimum_required(VERSION 3.16)
project(KiraWavTar VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
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

# mapbox/eternal (header-only compile-time map)
add_library(eternal INTERFACE)
target_include_directories(eternal SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/eternal/include)

# KiraCommonUtils (custom utility library)
add_subdirectory(lib/KiraCommonUtils)

# --- Main application ---
add_subdirectory(src)
