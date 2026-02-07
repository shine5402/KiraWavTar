#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <QString>
#include <kfr/all.hpp>
#include <memory>

#include "utils/Utils.h"

namespace AudioIO {

// Extended format info not present in kfr::audio_format in KFR 7
struct WavAudioFormat
{
    kfr::audio_format kfr_format;
    qint64 length = 0; // Length in samples per channel
    enum class Container { RIFF, RF64, W64, Unknown } container = Container::Unknown;

    // Convenience accessors to behave like a flat struct if needed, or just access members directly
    qint64 getLength() const { return length; }
};

// Returns format of the WAV file. Throws std::runtime_error on failure.
WavAudioFormat readWavFormat(const QString &fileName);

// Reads the entire WAV file into univector2d.
// Handles de-interleaving.
// Throws std::runtime_error on failure.
struct ReadResult
{
    kfr::univector2d<utils::sample_process_t> data;
    WavAudioFormat format;
};
ReadResult readWavFile(const QString &fileName);

// Writes the audio data to a WAV file.
// Handles interleaving and format conversion.
// Throws std::runtime_error on failure.
// Returns number of items written (samples * channels)
size_t writeWavFile(const QString &fileName, const kfr::univector2d<utils::sample_process_t> &data,
                    const WavAudioFormat &targetFormat);

// Overload for simpler usage if container/length details are defaults/implied
size_t writeWavFile(const QString &fileName, const kfr::univector2d<utils::sample_process_t> &data,
                    const kfr::audio_format &targetFormat);

} // namespace AudioIO

#endif
