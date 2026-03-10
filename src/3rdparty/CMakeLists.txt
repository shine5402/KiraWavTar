# --- Third-party libraries ---

# KFR (audio DSP library)
set(ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_DSP ON CACHE BOOL "" FORCE)
set(KFR_INSTALL_HEADERS OFF CACHE BOOL "" FORCE)
set(KFR_INSTALL_LIBRARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(kfr EXCLUDE_FROM_ALL)

# dr_libs
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE dr_libs)

# libFLAC (FLAC encoder/decoder)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(WITH_OGG OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(INSTALL_PKGCONFIG_MODULES OFF CACHE BOOL "" FORCE)
add_subdirectory(flac EXCLUDE_FROM_ALL)

# oneTBB (parallel pipeline framework)
# oneTBB must be built as a shared library — static builds are unsupported and
# cause thread-local-storage issues.  libFLAC (added above) sets
# BUILD_SHARED_LIBS=OFF in the cache, so we force it back to ON here.
set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TBB_STRICT OFF CACHE BOOL "" FORCE)
set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory(oneTBB EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)