#include "AudioIO.h"

#include <QCoreApplication>
#include <QFile>

#define DR_WAV_IMPLEMENTATION
#include "3rdparty/dr_libs/dr_wav.h"

namespace AudioIO {

using namespace utils;

// Manual implementation of f32 to s24 conversion since dr_libs doesn't export it
static void convert_f32_to_s24(drwav_uint8 *dst, const float *src, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        float sample = src[i];
        // Clamp
        if (sample > 1.0f)
            sample = 1.0f;
        if (sample < -1.0f)
            sample = -1.0f;
        // Scale to 24-bit range
        int32_t val = (int32_t)(sample * 8388607.0f);
        if (val > 8388607)
            val = 8388607;
        if (val < -8388608)
            val = -8388608;
        // Pack (little endian)
        dst[3 * i + 0] = (uint8_t)(val & 0xFF);
        dst[3 * i + 1] = (uint8_t)((val >> 8) & 0xFF);
        dst[3 * i + 2] = (uint8_t)((val >> 16) & 0xFF);
    }
}

kfr::audio_sample_type mapDrWavToKfrType(drwav_uint16 formatTag, drwav_uint16 bitsPerSample,
                                         drwav_uint16 validBitsPerSample)
{
    if (formatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
        if (bitsPerSample == 32)
            return kfr::audio_sample_type::f32;
        if (bitsPerSample == 64)
            return kfr::audio_sample_type::f64;
    } else if (formatTag == DR_WAVE_FORMAT_PCM) {
        // i8 and i64 are not supported in KFR 7 default enum
        if (bitsPerSample == 16)
            return kfr::audio_sample_type::i16;
        if (bitsPerSample == 24)
            return kfr::audio_sample_type::i24;
        if (bitsPerSample == 32)
            return kfr::audio_sample_type::i32;
    }
    return kfr::audio_sample_type::unknown;
}

WavAudioFormat readWavFormat(const QString &fileName)
{
    drwav wav;
    if (!drwav_init_file(&wav, fileName.toUtf8().constData(), nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    WavAudioFormat format;
    format.kfr_format.channels = wav.channels;
    format.kfr_format.samplerate = wav.sampleRate;
    format.length = wav.totalPCMFrameCount;
    // Use translatedFormatTag for extensible wavs
    format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample);

    // Map container format
    if (wav.container == drwav_container_riff)
        format.container = WavAudioFormat::Container::RIFF;
    else if (wav.container == drwav_container_rifx)
        format.container = WavAudioFormat::Container::RIFX;
    else if (wav.container == drwav_container_w64)
        format.container = WavAudioFormat::Container::W64;
    else if (wav.container == drwav_container_rf64)
        format.container = WavAudioFormat::Container::RF64;
    else if (wav.container == drwav_container_aiff)
        format.container = WavAudioFormat::Container::AIFF;
    else
        format.container = WavAudioFormat::Container::Unknown;

    drwav_uninit(&wav);
    return format;
}

static WavAudioFormat readWavFormatFromInitialized(const drwav &wav)
{
    WavAudioFormat format;
    format.kfr_format.channels = wav.channels;
    format.kfr_format.samplerate = wav.sampleRate;
    format.length = wav.totalPCMFrameCount;
    format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample);

    if (wav.container == drwav_container_riff)
        format.container = WavAudioFormat::Container::RIFF;
    else if (wav.container == drwav_container_rifx)
        format.container = WavAudioFormat::Container::RIFX;
    else if (wav.container == drwav_container_w64)
        format.container = WavAudioFormat::Container::W64;
    else if (wav.container == drwav_container_rf64)
        format.container = WavAudioFormat::Container::RF64;
    else if (wav.container == drwav_container_aiff)
        format.container = WavAudioFormat::Container::AIFF;
    else
        format.container = WavAudioFormat::Container::Unknown;

    return format;
}

template <typename T>
static void deinterleaveTo(kfr::univector2d<T> &dst, const T *interleaved, size_t framesRead, size_t channels)
{
    dst.resize(channels);
    for (size_t c = 0; c < channels; ++c)
        dst[c].resize(framesRead);

    for (size_t i = 0; i < framesRead; ++i) {
        for (size_t c = 0; c < channels; ++c) {
            dst[c][i] = interleaved[i * channels + c];
        }
    }
}

ReadResultF32 readWavFileF32(const QString &fileName)
{
    drwav wav;
    if (!drwav_init_file(&wav, fileName.toUtf8().constData(), nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    ReadResultF32 result;
    result.format = readWavFormatFromInitialized(wav);

    std::vector<float> interleavedBuffer(wav.totalPCMFrameCount * wav.channels);
    drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, interleavedBuffer.data());
    drwav_uninit(&wav);

    deinterleaveTo(result.data, interleavedBuffer.data(), static_cast<size_t>(framesRead),
                   result.format.kfr_format.channels);
    return result;
}

ReadResultF64 readWavFileF64(const QString &fileName)
{
    drwav wav;
    if (!drwav_init_file(&wav, fileName.toUtf8().constData(), nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    ReadResultF64 result;
    result.format = readWavFormatFromInitialized(wav);

    // This dr_wav version does not provide a direct f64 decode helper.
    // Decode to f32 then promote to double for internal processing.
    std::vector<float> interleavedF32(wav.totalPCMFrameCount * wav.channels);
    drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, interleavedF32.data());
    drwav_uninit(&wav);

    std::vector<double> interleavedF64(static_cast<size_t>(framesRead) * result.format.kfr_format.channels);
    for (size_t i = 0; i < interleavedF64.size(); ++i)
        interleavedF64[i] = static_cast<double>(interleavedF32[i]);

    deinterleaveTo(result.data, interleavedF64.data(), static_cast<size_t>(framesRead), result.format.kfr_format.channels);
    return result;
}

static drwav_data_format toDrWavDataFormat(const WavAudioFormat &targetFormat)
{
    drwav_data_format format;
    format.container = drwav_container_riff; // Default
    if (targetFormat.container == WavAudioFormat::Container::RIFF)
        format.container = drwav_container_riff;
    else if (targetFormat.container == WavAudioFormat::Container::RIFX)
        format.container = drwav_container_rifx;
    else if (targetFormat.container == WavAudioFormat::Container::W64)
        format.container = drwav_container_w64;
    else if (targetFormat.container == WavAudioFormat::Container::RF64)
        format.container = drwav_container_rf64;
    else if (targetFormat.container == WavAudioFormat::Container::AIFF)
        format.container = drwav_container_aiff;

    format.format = DR_WAVE_FORMAT_PCM;
    if (targetFormat.kfr_format.type == kfr::audio_sample_type::f32 ||
        targetFormat.kfr_format.type == kfr::audio_sample_type::f64)
    {
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    }

    format.channels = targetFormat.kfr_format.channels;
    format.sampleRate = targetFormat.kfr_format.samplerate;

    // determine bits per sample
    // i8 and i64 are removed/commented out in KFR 7
    if (targetFormat.kfr_format.type == kfr::audio_sample_type::i16)
        format.bitsPerSample = 16;
    else if (targetFormat.kfr_format.type == kfr::audio_sample_type::i24)
        format.bitsPerSample = 24;
    else if (targetFormat.kfr_format.type == kfr::audio_sample_type::i32)
        format.bitsPerSample = 32;
    else if (targetFormat.kfr_format.type == kfr::audio_sample_type::f32)
        format.bitsPerSample = 32;
    else if (targetFormat.kfr_format.type == kfr::audio_sample_type::f64)
        format.bitsPerSample = 64;
    else
        format.bitsPerSample = 16; // default

    return format;
}

template <typename T> static void interleaveFrom(std::vector<T> &dst, const kfr::univector2d<T> &data)
{
    size_t frameCount = data.empty() ? 0 : data[0].size();
    size_t channels = data.size();
    dst.resize(frameCount * channels);
    for (size_t i = 0; i < frameCount; ++i) {
        for (size_t c = 0; c < channels; ++c) {
            if (c < data.size() && i < data[c].size())
                dst[i * channels + c] = data[c][i];
            else
                dst[i * channels + c] = T(0);
        }
    }
}

static drwav initWriterOrThrow(drwav_data_format &format, const QString &fileName, size_t frameCount)
{
    drwav wav;
    if (!drwav_init_file_write_sequential_pcm_frames(&wav, fileName.toUtf8().constData(), &format, frameCount, nullptr))
    {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file for writing: %1").arg(fileName).toStdString());
    }
    return wav;
}

size_t writeWavFileF32(const QString &fileName, const kfr::univector2d<float> &data, const WavAudioFormat &targetFormat)
{
    if (data.empty())
        return 0;

    drwav_data_format format = toDrWavDataFormat(targetFormat);
    size_t frameCount = data[0].size();
    size_t channels = data.size();

    drwav wav = initWriterOrThrow(format, fileName, frameCount);

    std::vector<float> interleaved;
    interleaveFrom(interleaved, data);

    drwav_uint64 written = 0;
    if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 32) {
        written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
    } else if (format.format == DR_WAVE_FORMAT_PCM) {
        if (format.bitsPerSample == 16) {
            std::vector<drwav_int16> buf16(interleaved.size());
            drwav_f32_to_s16(buf16.data(), interleaved.data(), interleaved.size());
            written = drwav_write_pcm_frames(&wav, frameCount, buf16.data());
        } else if (format.bitsPerSample == 24) {
            std::vector<drwav_uint8> buf24(interleaved.size() * 3);
            convert_f32_to_s24(buf24.data(), interleaved.data(), interleaved.size());
            written = drwav_write_pcm_frames(&wav, frameCount, buf24.data());
        } else if (format.bitsPerSample == 32) {
            std::vector<drwav_int32> buf32(interleaved.size());
            drwav_f32_to_s32(buf32.data(), interleaved.data(), interleaved.size());
            written = drwav_write_pcm_frames(&wav, frameCount, buf32.data());
        }
    } else if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 64) {
        std::vector<double> buf64(interleaved.size());
        for (size_t i = 0; i < interleaved.size(); ++i)
            buf64[i] = interleaved[i];
        written = drwav_write_pcm_frames(&wav, frameCount, buf64.data());
    }

    drwav_uninit(&wav);
    return written * channels;
}

size_t writeWavFileF64(const QString &fileName, const kfr::univector2d<double> &data,
                       const WavAudioFormat &targetFormat)
{
    if (data.empty())
        return 0;

    drwav_data_format format = toDrWavDataFormat(targetFormat);
    size_t frameCount = data[0].size();
    size_t channels = data.size();

    drwav wav = initWriterOrThrow(format, fileName, frameCount);

    drwav_uint64 written = 0;
    if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 64) {
        std::vector<double> interleaved;
        interleaveFrom(interleaved, data);
        written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
    } else {
        // For non-f64 outputs, convert via float for the final quantization.
        std::vector<float> interleavedF32(frameCount * channels);
        for (size_t i = 0; i < frameCount; ++i) {
            for (size_t c = 0; c < channels; ++c) {
                double v = (c < data.size() && i < data[c].size()) ? data[c][i] : 0.0;
                interleavedF32[i * channels + c] = static_cast<float>(v);
            }
        }

        if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 32) {
            written = drwav_write_pcm_frames(&wav, frameCount, interleavedF32.data());
        } else if (format.format == DR_WAVE_FORMAT_PCM) {
            if (format.bitsPerSample == 16) {
                std::vector<drwav_int16> buf16(interleavedF32.size());
                drwav_f32_to_s16(buf16.data(), interleavedF32.data(), interleavedF32.size());
                written = drwav_write_pcm_frames(&wav, frameCount, buf16.data());
            } else if (format.bitsPerSample == 24) {
                std::vector<drwav_uint8> buf24(interleavedF32.size() * 3);
                convert_f32_to_s24(buf24.data(), interleavedF32.data(), interleavedF32.size());
                written = drwav_write_pcm_frames(&wav, frameCount, buf24.data());
            } else if (format.bitsPerSample == 32) {
                std::vector<drwav_int32> buf32(interleavedF32.size());
                drwav_f32_to_s32(buf32.data(), interleavedF32.data(), interleavedF32.size());
                written = drwav_write_pcm_frames(&wav, frameCount, buf32.data());
            }
        }
    }

    drwav_uninit(&wav);
    return written * channels;
}

size_t writeWavFileF32(const QString &fileName, const kfr::univector2d<float> &data,
                       const kfr::audio_format &targetFormat)
{
    WavAudioFormat fmt;
    fmt.kfr_format = targetFormat;
    fmt.container = WavAudioFormat::Container::RIFF; // Default
    return writeWavFileF32(fileName, data, fmt);
}

size_t writeWavFileF64(const QString &fileName, const kfr::univector2d<double> &data,
                       const kfr::audio_format &targetFormat)
{
    WavAudioFormat fmt;
    fmt.kfr_format = targetFormat;
    fmt.container = WavAudioFormat::Container::RIFF; // Default
    return writeWavFileF64(fileName, data, fmt);
}
} // namespace AudioIO
