# Worker - Audio Processing
target_sources(kirawavtar_core PRIVATE
    AudioIO.cpp
    AudioIO.h
    AudioCombine.cpp
    AudioCombine.h
    AudioExtract.cpp
    AudioExtract.h
)

target_include_directories(kirawavtar_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
