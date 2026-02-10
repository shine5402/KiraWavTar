#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <QString>
#include <kfr/all.hpp>
#include <memory>

#include "utils/Utils.h"

namespace AudioIO {

// Extended format info not present in kfr::audio_format in KFR 7
struct AudioFormat
{
    kfr::audio_format kfr_format;
    qint64 length = 0; // Length in samples per channel
    enum class Container { RIFF, RIFX, W64, RF64, AIFF, FLAC, Unknown } container = Container::Unknown;

    // Helper to check if this is a FLAC container
    bool isFlac() const { return container == Container::FLAC; }

    // Convenience accessors to behave like a flat struct if needed, or just access members directly
    qint64 getLength() const { return length; }
};

// Returns format of the audio file (WAV or FLAC). Throws std::runtime_error on failure.
// Dispatches by file extension: .flac -> FLAC decoder, otherwise -> WAV decoder.
AudioFormat readAudioFormat(const QString &fileName);

// Reads the entire audio file into univector2d.
// Handles de-interleaving. Dispatches by file extension.
// Throws std::runtime_error on failure.
struct ReadResultF32
{
    kfr::univector2d<float> data;
    AudioFormat format;
};

struct ReadResultF64
{
    kfr::univector2d<double> data;
    AudioFormat format;
};

ReadResultF32 readAudioFileF32(const QString &fileName);
ReadResultF64 readAudioFileF64(const QString &fileName);

// Writes the audio data to a file (WAV or FLAC).
// Dispatches by container format in targetFormat.
// Handles interleaving and format conversion.
// Throws std::runtime_error on failure.
// Returns number of items written (samples * channels)
size_t writeAudioFileF32(const QString &fileName, const kfr::univector2d<float> &data, const AudioFormat &targetFormat);
size_t writeAudioFileF64(const QString &fileName, const kfr::univector2d<double> &data,
                         const AudioFormat &targetFormat);

// Overload for simpler usage if container/length details are defaults/implied
size_t writeAudioFileF32(const QString &fileName, const kfr::univector2d<float> &data,
                         const kfr::audio_format &targetFormat);
size_t writeAudioFileF64(const QString &fileName, const kfr::univector2d<double> &data,
                         const kfr::audio_format &targetFormat);

} // namespace AudioIO

#endif
