# --- Third-party libraries ---

# KFR (audio DSP library)
set(ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DSP ON CACHE BOOL "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(KFR_INSTALL_HEADERS OFF CACHE BOOL "" FORCE)
set(KFR_INSTALL_LIBRARIES OFF CACHE BOOL "" FORCE)
add_subdirectory(kfr EXCLUDE_FROM_ALL)

# dr_libs
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE dr_libs)