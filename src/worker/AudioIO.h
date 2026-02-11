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

// --- Streaming writer for chunked output (used by combine pipeline) ---

class StreamingAudioWriter {
public:
    StreamingAudioWriter();
    ~StreamingAudioWriter();

    // Non-copyable, movable
    StreamingAudioWriter(const StreamingAudioWriter &) = delete;
    StreamingAudioWriter &operator=(const StreamingAudioWriter &) = delete;
    StreamingAudioWriter(StreamingAudioWriter &&) noexcept;
    StreamingAudioWriter &operator=(StreamingAudioWriter &&) noexcept;

    // Opens the output file. totalFrameCountHint is optional (pass 0 if unknown).
    void open(const QString &fileName, const AudioFormat &targetFormat,
              qint64 totalFrameCountHint = 0);

    // Write a chunk of planar audio frames. Can be called repeatedly.
    void writeFramesF32(const kfr::univector2d<float> &data);
    void writeFramesF64(const kfr::univector2d<double> &data);

    // Write N frames of silence (zeros).
    void writeSilence(qint64 frameCount);

    // Total frames written so far.
    qint64 framesWritten() const { return m_framesWritten; }

    // Finalize and close. Called automatically by destructor.
    void close();

    bool isOpen() const { return m_isOpen; }

private:
    AudioFormat m_format{};
    qint64 m_framesWritten = 0;
    bool m_isOpen = false;

    // WAV backend (dr_libs) — forward declared, actual drwav stored in impl
    struct WavState;
    std::unique_ptr<WavState> m_wavState;
    bool m_isWav = false;

    // FLAC backend (libFLAC) — opaque pointer
    struct FlacState;
    std::unique_ptr<FlacState> m_flacState;

    // Dithering state persists across chunks for TPDF continuity
    struct DitherState;
    std::unique_ptr<DitherState> m_ditherState;
};

// --- Streaming reader for chunked input (used by extract pipeline) ---

class StreamingAudioReader {
public:
    StreamingAudioReader();
    ~StreamingAudioReader();

    // Non-copyable, movable
    StreamingAudioReader(const StreamingAudioReader &) = delete;
    StreamingAudioReader &operator=(const StreamingAudioReader &) = delete;
    StreamingAudioReader(StreamingAudioReader &&) noexcept;
    StreamingAudioReader &operator=(StreamingAudioReader &&) noexcept;

    // Opens the audio file. Dispatches by extension (.flac → FLAC, else → WAV).
    void open(const QString &fileName);

    AudioFormat format() const { return m_format; }

    // Seek to an absolute PCM frame position. Returns true on success.
    bool seekToFrame(qint64 frame);

    // Read frameCount frames starting from current position, returned as planar float/double.
    kfr::univector2d<float> readFramesF32(qint64 frameCount);
    kfr::univector2d<double> readFramesF64(qint64 frameCount);

    void close();
    bool isOpen() const { return m_isOpen; }

private:
    AudioFormat m_format{};
    bool m_isOpen = false;
    bool m_isWav = false;

    struct WavState;
    std::unique_ptr<WavState> m_wavState;

    struct FlacState;
    std::unique_ptr<FlacState> m_flacState;
};

} // namespace AudioIO

#endif
