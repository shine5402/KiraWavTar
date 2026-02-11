# Worker - Audio Processing
target_sources(KiraWAVTar PRIVATE
    AudioIO.cpp
    AudioIO.h
    AudioCombine.cpp
    AudioCombine.h
    AudioExtract.cpp
    AudioExtract.h
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
