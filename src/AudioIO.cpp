#include "AudioIO.h"
#define DR_WAV_IMPLEMENTATION
#include "../3rdparty/dr_libs/dr_wav.h"
#include <QCoreApplication>
#include <QFile>

namespace AudioIO {

    using namespace wavtar_defines;

    // Manual implementation of f32 to s24 conversion since dr_libs doesn't export it
    static void convert_f32_to_s24(drwav_uint8* dst, const float* src, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            float sample = src[i];
            // Clamp
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            // Scale to 24-bit range
            int32_t val = (int32_t)(sample * 8388607.0f);
            if (val > 8388607) val = 8388607;
            if (val < -8388608) val = -8388608;
            // Pack (little endian)
            dst[3*i+0] = (uint8_t)(val & 0xFF);
            dst[3*i+1] = (uint8_t)((val >> 8) & 0xFF);
            dst[3*i+2] = (uint8_t)((val >> 16) & 0xFF);
        }
    }

    kfr::audio_sample_type mapDrWavToKfrType(drwav_uint16 formatTag, drwav_uint16 bitsPerSample, drwav_uint16 validBitsPerSample) {
        if (formatTag == DR_WAVE_FORMAT_IEEE_FLOAT) {
            if (bitsPerSample == 32) return kfr::audio_sample_type::f32;
            if (bitsPerSample == 64) return kfr::audio_sample_type::f64;
        } else if (formatTag == DR_WAVE_FORMAT_PCM) {
            // i8 and i64 are not supported in KFR 7 default enum
            if (bitsPerSample == 16) return kfr::audio_sample_type::i16;
            if (bitsPerSample == 24) return kfr::audio_sample_type::i24;
            if (bitsPerSample == 32) return kfr::audio_sample_type::i32;
        }
        return kfr::audio_sample_type::unknown;
    }

    WavAudioFormat readWavFormat(const QString& fileName) {
        drwav wav;
        if (!drwav_init_file(&wav, fileName.toUtf8().constData(), nullptr)) {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName));
        }

        WavAudioFormat format;
        format.kfr_format.channels = wav.channels;
        format.kfr_format.samplerate = wav.sampleRate;
        format.length = wav.totalPCMFrameCount;
        // Use translatedFormatTag for extensible wavs
        format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample); 
        
        // Map container format
        if (wav.container == drwav_container_riff) format.container = WavAudioFormat::Container::RIFF;
        else if (wav.container == drwav_container_w64) format.container = WavAudioFormat::Container::W64;
        else if (wav.container == drwav_container_rf64) format.container = WavAudioFormat::Container::RF64;
        else format.container = WavAudioFormat::Container::Unknown;

        drwav_uninit(&wav);
        return format;
    }

    ReadResult readWavFile(const QString& fileName) {
        drwav wav;
        if (!drwav_init_file(&wav, fileName.toUtf8().constData(), nullptr)) {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName));
        }

        ReadResult result;
        result.format.kfr_format.channels = wav.channels;
        result.format.kfr_format.samplerate = wav.sampleRate;
        result.format.length = wav.totalPCMFrameCount;
        result.format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample);

        if (wav.container == drwav_container_riff) result.format.container = WavAudioFormat::Container::RIFF;
        else if (wav.container == drwav_container_w64) result.format.container = WavAudioFormat::Container::W64;
        else if (wav.container == drwav_container_rf64) result.format.container = WavAudioFormat::Container::RF64;
        else result.format.container = WavAudioFormat::Container::Unknown;

        // Allocate buffer
        std::vector<float> interleavedBuffer(wav.totalPCMFrameCount * wav.channels);
        
        // Read as float. dr_wav converts automatically.
        drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, interleavedBuffer.data());
        
        drwav_uninit(&wav);

        // De-interleave
        result.data.resize(wav.channels);
        for(size_t c = 0; c < wav.channels; ++c) {
            result.data[c].resize(framesRead);
        }

        for (size_t i = 0; i < framesRead; ++i) {
            for (size_t c = 0; c < wav.channels; ++c) {
                result.data[c][i] = interleavedBuffer[i * wav.channels + c];
            }
        }

        return result;
    }

    size_t writeWavFile(const QString& fileName, const kfr::univector2d<wavtar_defines::sample_process_t>& data, const WavAudioFormat& targetFormat) {
        drwav_data_format format;
        format.container = drwav_container_riff; // Default
        if (targetFormat.container == WavAudioFormat::Container::W64) format.container = drwav_container_w64;
        if (targetFormat.container == WavAudioFormat::Container::RF64) format.container = drwav_container_rf64;

        format.format = DR_WAVE_FORMAT_PCM; 
        if (targetFormat.kfr_format.type == kfr::audio_sample_type::f32 || targetFormat.kfr_format.type == kfr::audio_sample_type::f64) {
             format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        }

        format.channels = targetFormat.kfr_format.channels;
        format.sampleRate = targetFormat.kfr_format.samplerate;
        
        // determine bits per sample
        // i8 and i64 are removed/commented out in KFR 7
        if (targetFormat.kfr_format.type == kfr::audio_sample_type::i16) format.bitsPerSample = 16;
        else if (targetFormat.kfr_format.type == kfr::audio_sample_type::i24) format.bitsPerSample = 24;
        else if (targetFormat.kfr_format.type == kfr::audio_sample_type::i32) format.bitsPerSample = 32;
        else if (targetFormat.kfr_format.type == kfr::audio_sample_type::f32) format.bitsPerSample = 32;
        else if (targetFormat.kfr_format.type == kfr::audio_sample_type::f64) format.bitsPerSample = 64;
        else format.bitsPerSample = 16; // default

        drwav wav;
        if (!drwav_init_file_write_sequential_pcm_frames(&wav, fileName.toUtf8().constData(), &format, data[0].size(), nullptr)) {
             throw wavtar_exceptions::runtime_error(QCoreApplication::translate("AudioIO", "Failed to open file for writing: %1").arg(fileName));
        }
        
        size_t frameCount = data[0].size(); 
        size_t channels = data.size();
        
        std::vector<float> interleaved(frameCount * channels);
        for (size_t i = 0; i < frameCount; ++i) {
            for (size_t c = 0; c < channels; ++c) {
                if (c < data.size() && i < data[c].size())
                    interleaved[i * channels + c] = data[c][i];
                else
                    interleaved[i * channels + c] = 0.0f;
            }
        }
        
        drwav_uint64 written = 0;

        if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 32) {
             written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
        } else {
            if (format.format == DR_WAVE_FORMAT_PCM) {
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
                 for(size_t i=0; i<interleaved.size(); ++i) buf64[i] = interleaved[i];
                 written = drwav_write_pcm_frames(&wav, frameCount, buf64.data());
            }
        }
        
        drwav_uninit(&wav);
        return written * channels;
    }

    size_t writeWavFile(const QString& fileName, const kfr::univector2d<wavtar_defines::sample_process_t>& data, const kfr::audio_format& targetFormat) {
        WavAudioFormat fmt;
        fmt.kfr_format = targetFormat;
        fmt.container = WavAudioFormat::Container::RIFF; // Default
        return writeWavFile(fileName, data, fmt);
    }
}
