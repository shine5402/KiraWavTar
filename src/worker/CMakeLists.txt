# Worker - Audio Processing
target_sources(KiraWAVTar PRIVATE
    AudioIO.cpp
    AudioIO.h
    WavCombine.cpp
    WavCombine.h
    WavExtract.cpp
    WavExtract.h
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
