#include "AudioIO.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#define DR_WAV_IMPLEMENTATION
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>

#include "3rdparty/dr_libs/dr_wav.h"

namespace AudioIO {

using namespace utils;

// --- Helper: detect FLAC by file extension ---
static bool isFlacFileName(const QString &fileName)
{
    return fileName.endsWith(QLatin1String(".flac"), Qt::CaseInsensitive);
}

// On Windows, drwav_init_file() uses fopen() which expects ANSI paths, not UTF-8.
// We must use the _w (wide-char) variants on Windows for proper Unicode path support.
#ifdef Q_OS_WIN
static drwav_bool32 drwav_init_file_compat(drwav *pWav, const QString &fileName,
                                           const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file_w(pWav, reinterpret_cast<const wchar_t *>(fileName.utf16()), pAllocationCallbacks);
}

static drwav_bool32
drwav_init_file_write_sequential_pcm_frames_compat(drwav *pWav, const QString &fileName,
                                                   const drwav_data_format *pFormat, drwav_uint64 totalPCMFrameCount,
                                                   const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file_write_sequential_pcm_frames_w(pWav, reinterpret_cast<const wchar_t *>(fileName.utf16()),
                                                         pFormat, totalPCMFrameCount, pAllocationCallbacks);
}
#else
static drwav_bool32 drwav_init_file_compat(drwav *pWav, const QString &fileName,
                                           const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file(pWav, fileName.toUtf8().constData(), pAllocationCallbacks);
}

static drwav_bool32
drwav_init_file_write_sequential_pcm_frames_compat(drwav *pWav, const QString &fileName,
                                                   const drwav_data_format *pFormat, drwav_uint64 totalPCMFrameCount,
                                                   const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file_write_sequential_pcm_frames(pWav, fileName.toUtf8().constData(), pFormat, totalPCMFrameCount,
                                                       pAllocationCallbacks);
}
#endif

// Helper: build an array of const fbase* pointers from univector2d<double> (fbase = double)
static std::vector<const kfr::fbase *> planarPointers(const kfr::univector2d<double> &data)
{
    std::vector<const kfr::fbase *> ptrs(data.size());
    for (size_t c = 0; c < data.size(); ++c)
        ptrs[c] = data[c].data();
    return ptrs;
}

// Helper: promote float planar data to double planar data (required because kfr::fbase = double)
static kfr::univector2d<double> promoteToDouble(const kfr::univector2d<float> &data)
{
    kfr::univector2d<double> out(data.size());
    for (size_t c = 0; c < data.size(); ++c) {
        out[c].resize(data[c].size());
        for (size_t i = 0; i < data[c].size(); ++i)
            out[c][i] = static_cast<double>(data[c][i]);
    }
    return out;
}

// ---------- WAV-specific implementation (via drwav) ----------

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

AudioFormat readWavFormat(const QString &fileName)
{
    drwav wav;
    if (!drwav_init_file_compat(&wav, fileName, nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    AudioFormat format;
    format.kfr_format.channels = wav.channels;
    format.kfr_format.samplerate = wav.sampleRate;
    format.length = wav.totalPCMFrameCount;
    // Use translatedFormatTag for extensible wavs
    format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample);

    // Map container format
    if (wav.container == drwav_container_riff)
        format.container = AudioFormat::Container::RIFF;
    else if (wav.container == drwav_container_rifx)
        format.container = AudioFormat::Container::RIFX;
    else if (wav.container == drwav_container_w64)
        format.container = AudioFormat::Container::W64;
    else if (wav.container == drwav_container_rf64)
        format.container = AudioFormat::Container::RF64;
    else if (wav.container == drwav_container_aiff)
        format.container = AudioFormat::Container::AIFF;
    else
        format.container = AudioFormat::Container::Unknown;

    drwav_uninit(&wav);
    return format;
}

static AudioFormat readAudioFormatFromInitialized(const drwav &wav)
{
    AudioFormat format;
    format.kfr_format.channels = wav.channels;
    format.kfr_format.samplerate = wav.sampleRate;
    format.length = wav.totalPCMFrameCount;
    format.kfr_format.type = mapDrWavToKfrType(wav.translatedFormatTag, wav.bitsPerSample, wav.bitsPerSample);

    if (wav.container == drwav_container_riff)
        format.container = AudioFormat::Container::RIFF;
    else if (wav.container == drwav_container_rifx)
        format.container = AudioFormat::Container::RIFX;
    else if (wav.container == drwav_container_w64)
        format.container = AudioFormat::Container::W64;
    else if (wav.container == drwav_container_rf64)
        format.container = AudioFormat::Container::RF64;
    else if (wav.container == drwav_container_aiff)
        format.container = AudioFormat::Container::AIFF;
    else
        format.container = AudioFormat::Container::Unknown;

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
    if (!drwav_init_file_compat(&wav, fileName, nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    ReadResultF32 result;
    result.format = readAudioFormatFromInitialized(wav);

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
    if (!drwav_init_file_compat(&wav, fileName, nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file: %1").arg(fileName).toStdString());
    }

    ReadResultF64 result;
    result.format = readAudioFormatFromInitialized(wav);

    // This dr_wav version does not provide a direct f64 decode helper.
    // Decode to f32 then promote to double for internal processing.
    std::vector<float> interleavedF32(wav.totalPCMFrameCount * wav.channels);
    drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, interleavedF32.data());
    drwav_uninit(&wav);

    std::vector<double> interleavedF64(static_cast<size_t>(framesRead) * result.format.kfr_format.channels);
    for (size_t i = 0; i < interleavedF64.size(); ++i)
        interleavedF64[i] = static_cast<double>(interleavedF32[i]);

    deinterleaveTo(result.data, interleavedF64.data(), static_cast<size_t>(framesRead),
                   result.format.kfr_format.channels);
    return result;
}

static drwav_data_format toDrWavDataFormat(const AudioFormat &targetFormat)
{
    drwav_data_format format;
    format.container = drwav_container_riff; // Default
    if (targetFormat.container == AudioFormat::Container::RIFF)
        format.container = drwav_container_riff;
    else if (targetFormat.container == AudioFormat::Container::RIFX)
        format.container = drwav_container_rifx;
    else if (targetFormat.container == AudioFormat::Container::W64)
        format.container = drwav_container_w64;
    else if (targetFormat.container == AudioFormat::Container::RF64)
        format.container = drwav_container_rf64;
    else if (targetFormat.container == AudioFormat::Container::AIFF)
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
    if (!drwav_init_file_write_sequential_pcm_frames_compat(&wav, fileName, &format, frameCount, nullptr)) {
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open file for writing: %1").arg(fileName).toStdString());
    }
    return wav;
}

// Write interleaved PCM via kfr::samples_store (handles dithering + quantization).
// The planar overload interleaves, applies TPDF dither, clamps, and quantizes in one pass.
static drwav_uint64 writePcmWithDither(drwav &wav, const kfr::univector2d<double> &data,
                                       const drwav_data_format &format)
{
    size_t frameCount = data[0].size();
    size_t channels = data.size();
    size_t totalSamples = frameCount * channels;
    auto ptrs = planarPointers(data);
    kfr::audio_quantization quant(format.bitsPerSample, kfr::audio_dithering::triangular);

    if (format.bitsPerSample == 16) {
        std::vector<kfr::i16> buf(totalSamples);
        kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
        return drwav_write_pcm_frames(&wav, frameCount, buf.data());
    } else if (format.bitsPerSample == 24) {
        std::vector<kfr::i24> buf(totalSamples);
        kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
        return drwav_write_pcm_frames(&wav, frameCount, buf.data());
    } else if (format.bitsPerSample == 32) {
        std::vector<kfr::i32> buf(totalSamples);
        kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
        return drwav_write_pcm_frames(&wav, frameCount, buf.data());
    }
    return 0;
}

size_t writeWavFileF32(const QString &fileName, const kfr::univector2d<float> &data, const AudioFormat &targetFormat)
{
    if (data.empty())
        return 0;

    drwav_data_format format = toDrWavDataFormat(targetFormat);
    size_t frameCount = data[0].size();
    size_t channels = data.size();

    drwav wav = initWriterOrThrow(format, fileName, frameCount);

    drwav_uint64 written = 0;
    if (format.format == DR_WAVE_FORMAT_PCM) {
        // Promote to double (kfr::fbase) for KFR's dithered quantization
        auto dataDouble = promoteToDouble(data);
        written = writePcmWithDither(wav, dataDouble, format);
    } else if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 32) {
        std::vector<float> interleaved;
        interleaveFrom(interleaved, data);
        written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
    } else if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 64) {
        auto dataDouble = promoteToDouble(data);
        std::vector<double> interleaved;
        interleaveFrom(interleaved, dataDouble);
        written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
    }

    drwav_uninit(&wav);
    return written * channels;
}

size_t writeWavFileF64(const QString &fileName, const kfr::univector2d<double> &data, const AudioFormat &targetFormat)
{
    if (data.empty())
        return 0;

    drwav_data_format format = toDrWavDataFormat(targetFormat);
    size_t frameCount = data[0].size();
    size_t channels = data.size();

    drwav wav = initWriterOrThrow(format, fileName, frameCount);

    drwav_uint64 written = 0;
    if (format.format == DR_WAVE_FORMAT_PCM) {
        // data is already double = kfr::fbase, use KFR's dithered quantization directly
        written = writePcmWithDither(wav, data, format);
    } else if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 64) {
        std::vector<double> interleaved;
        interleaveFrom(interleaved, data);
        written = drwav_write_pcm_frames(&wav, frameCount, interleaved.data());
    } else if (format.format == DR_WAVE_FORMAT_IEEE_FLOAT && format.bitsPerSample == 32) {
        // double → float, then write
        std::vector<float> interleavedF32(frameCount * channels);
        for (size_t i = 0; i < frameCount; ++i) {
            for (size_t c = 0; c < channels; ++c) {
                double v = (c < data.size() && i < data[c].size()) ? data[c][i] : 0.0;
                interleavedF32[i * channels + c] = static_cast<float>(v);
            }
        }
        written = drwav_write_pcm_frames(&wav, frameCount, interleavedF32.data());
    }

    drwav_uninit(&wav);
    return written * channels;
}

// ---------- FLAC-specific implementation (via libFLAC) ----------

static kfr::audio_sample_type mapFlacBitsToKfrType(unsigned bitsPerSample)
{
    if (bitsPerSample <= 16)
        return kfr::audio_sample_type::i16;
    if (bitsPerSample <= 24)
        return kfr::audio_sample_type::i24;
    return kfr::audio_sample_type::i32;
}

static unsigned kfrTypeToFlacBits(kfr::audio_sample_type type)
{
    switch (type) {
    case kfr::audio_sample_type::i16:
        return 16;
    case kfr::audio_sample_type::i24:
        return 24;
    case kfr::audio_sample_type::i32:
        return 32;
    default:
        return 24; // safe default
    }
}

// Read FLAC format metadata only
static AudioFormat readFlacFormat(const QString &fileName)
{
    struct FlacMetadata
    {
        AudioFormat format;
        bool gotStreamInfo = false;
    };

    FlacMetadata meta;

    auto *decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        throw std::runtime_error(QCoreApplication::translate("AudioIO", "Failed to create FLAC decoder for: %1")
                                     .arg(fileName)
                                     .toStdString());
    }

    auto metadataCallback = [](const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *clientData) {
        auto *m = static_cast<FlacMetadata *>(clientData);
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            const auto &info = metadata->data.stream_info;
            m->format.kfr_format.samplerate = info.sample_rate;
            m->format.kfr_format.channels = info.channels;
            m->format.kfr_format.type = mapFlacBitsToKfrType(info.bits_per_sample);
            m->format.length = static_cast<qint64>(info.total_samples);
            m->format.container = AudioFormat::Container::FLAC;
            m->gotStreamInfo = true;
        }
    };

    auto errorCallback = [](const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *) {
    };

    // We don't need a write callback since we only read metadata
    auto writeCallback = [](const FLAC__StreamDecoder *, const FLAC__Frame *, const FLAC__int32 *const *,
                            void *) -> FLAC__StreamDecoderWriteStatus {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT; // Stop after metadata
    };

    QByteArray path = fileName.toUtf8();
    auto initStatus = FLAC__stream_decoder_init_file(decoder, path.constData(), writeCallback, metadataCallback,
                                                     errorCallback, &meta);
    if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        FLAC__stream_decoder_delete(decoder);
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open FLAC file: %1").arg(fileName).toStdString());
    }

    FLAC__stream_decoder_process_until_end_of_metadata(decoder);
    FLAC__stream_decoder_finish(decoder);
    FLAC__stream_decoder_delete(decoder);

    if (!meta.gotStreamInfo) {
        throw std::runtime_error(QCoreApplication::translate("AudioIO", "Failed to read FLAC metadata from: %1")
                                     .arg(fileName)
                                     .toStdString());
    }

    return meta.format;
}

// Helper struct for FLAC decoding context
template <typename T> struct FlacDecodeContext
{
    kfr::univector2d<T> data;
    AudioFormat format;
    unsigned bitsPerSample = 0;
    bool gotStreamInfo = false;
    size_t writePos = 0;
    QString errorMsg;
};

template <typename T>
static FLAC__StreamDecoderWriteStatus flacWriteCallback(const FLAC__StreamDecoder *, const FLAC__Frame *frame,
                                                        const FLAC__int32 *const buffer[], void *clientData)
{
    auto *ctx = static_cast<FlacDecodeContext<T> *>(clientData);
    unsigned channels = frame->header.channels;
    unsigned blocksize = frame->header.blocksize;
    unsigned bps = ctx->bitsPerSample;

    // Scale factor: FLAC delivers samples as signed integers in FLAC__int32.
    // We need to normalize to [-1.0, 1.0) range.
    double scale = 1.0 / static_cast<double>(1ULL << (bps - 1));

    for (unsigned c = 0; c < channels && c < ctx->data.size(); ++c) {
        for (unsigned i = 0; i < blocksize; ++i) {
            size_t pos = ctx->writePos + i;
            if (pos < ctx->data[c].size()) {
                ctx->data[c][pos] = static_cast<T>(buffer[c][i] * scale);
            }
        }
    }
    ctx->writePos += blocksize;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

template <typename T>
static void flacMetadataCallback(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *clientData)
{
    auto *ctx = static_cast<FlacDecodeContext<T> *>(clientData);
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        const auto &info = metadata->data.stream_info;
        ctx->format.kfr_format.samplerate = info.sample_rate;
        ctx->format.kfr_format.channels = info.channels;
        ctx->format.kfr_format.type = mapFlacBitsToKfrType(info.bits_per_sample);
        ctx->format.length = static_cast<qint64>(info.total_samples);
        ctx->format.container = AudioFormat::Container::FLAC;
        ctx->bitsPerSample = info.bits_per_sample;
        ctx->gotStreamInfo = true;

        // Pre-allocate output buffers
        ctx->data.resize(info.channels);
        for (unsigned c = 0; c < info.channels; ++c) {
            ctx->data[c].resize(info.total_samples);
        }
    }
}

template <typename T>
static void flacErrorCallback(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *clientData)
{
    auto *ctx = static_cast<FlacDecodeContext<T> *>(clientData);
    ctx->errorMsg = QString("FLAC decode error: %1").arg(static_cast<int>(status));
}

template <typename T> static auto readFlacFileTyped(const QString &fileName)
{
    FlacDecodeContext<T> ctx;

    auto *decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        throw std::runtime_error(QCoreApplication::translate("AudioIO", "Failed to create FLAC decoder for: %1")
                                     .arg(fileName)
                                     .toStdString());
    }

    QByteArray path = fileName.toUtf8();
    auto initStatus = FLAC__stream_decoder_init_file(decoder, path.constData(), flacWriteCallback<T>,
                                                     flacMetadataCallback<T>, flacErrorCallback<T>, &ctx);
    if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        FLAC__stream_decoder_delete(decoder);
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to open FLAC file: %1").arg(fileName).toStdString());
    }

    if (!FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);
        if (!ctx.errorMsg.isEmpty()) {
            throw std::runtime_error(ctx.errorMsg.toStdString());
        }
        throw std::runtime_error(
            QCoreApplication::translate("AudioIO", "Failed to decode FLAC file: %1").arg(fileName).toStdString());
    }

    FLAC__stream_decoder_finish(decoder);
    FLAC__stream_decoder_delete(decoder);

    // Trim buffers to actual write position (in case total_samples was 0/unknown)
    for (size_t c = 0; c < ctx.data.size(); ++c) {
        if (ctx.data[c].size() > ctx.writePos) {
            ctx.data[c].resize(ctx.writePos);
        }
    }
    ctx.format.length = static_cast<qint64>(ctx.writePos);

    return ctx;
}

static ReadResultF32 readFlacFileF32(const QString &fileName)
{
    auto ctx = readFlacFileTyped<float>(fileName);
    return {std::move(ctx.data), ctx.format};
}

static ReadResultF64 readFlacFileF64(const QString &fileName)
{
    auto ctx = readFlacFileTyped<double>(fileName);
    return {std::move(ctx.data), ctx.format};
}

// Write FLAC file from interleaved float/double data
template <typename T>
static size_t writeFlacFileTyped(const QString &fileName, const kfr::univector2d<T> &data,
                                 const AudioFormat &targetFormat)
{
    if (data.empty())
        return 0;

    size_t frameCount = data[0].size();
    size_t channels = data.size();
    unsigned bitsPerSample = kfrTypeToFlacBits(targetFormat.kfr_format.type);

    auto *encoder = FLAC__stream_encoder_new();
    if (!encoder) {
        throw std::runtime_error(QCoreApplication::translate("AudioIO", "Failed to create FLAC encoder for: %1")
                                     .arg(fileName)
                                     .toStdString());
    }

    FLAC__stream_encoder_set_channels(encoder, static_cast<unsigned>(channels));
    FLAC__stream_encoder_set_bits_per_sample(encoder, bitsPerSample);
    FLAC__stream_encoder_set_sample_rate(encoder, static_cast<unsigned>(targetFormat.kfr_format.samplerate));
    FLAC__stream_encoder_set_compression_level(encoder, 5); // Default compression
    FLAC__stream_encoder_set_total_samples_estimate(encoder, static_cast<FLAC__uint64>(frameCount));

    QByteArray path = fileName.toUtf8();
    auto initStatus = FLAC__stream_encoder_init_file(encoder, path.constData(), nullptr, nullptr);
    if (initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        FLAC__stream_encoder_delete(encoder);
        throw std::runtime_error(QCoreApplication::translate("AudioIO", "Failed to open FLAC file for writing: %1")
                                     .arg(fileName)
                                     .toStdString());
    }

    // Convert float/double data to FLAC__int32 interleaved and encode in chunks,
    // using KFR's audio_quantization for TPDF dithering.
    constexpr size_t chunkFrames = 4096;
    double scale = static_cast<double>(1ULL << (bitsPerSample - 1));
    double maxVal = scale - 1.0;
    double minVal = -scale;

    kfr::audio_quantization quant(bitsPerSample, kfr::audio_dithering::triangular);

    std::vector<FLAC__int32> buffer(chunkFrames * channels);

    for (size_t offset = 0; offset < frameCount; offset += chunkFrames) {
        size_t thisChunk = std::min(chunkFrames, frameCount - offset);

        for (size_t i = 0; i < thisChunk; ++i) {
            for (size_t c = 0; c < channels; ++c) {
                double sample = static_cast<double>(data[c][offset + i]);
                // Add TPDF dither, clamp to [-1, +1], then scale to integer range
                double dithered = std::clamp(sample + quant.dither(), -1.0, 1.0);
                double scaled = dithered * scale;
                if (scaled > maxVal)
                    scaled = maxVal;
                if (scaled < minVal)
                    scaled = minVal;
                buffer[i * channels + c] = static_cast<FLAC__int32>(std::llround(scaled));
            }
        }

        if (!FLAC__stream_encoder_process_interleaved(encoder, buffer.data(), static_cast<unsigned>(thisChunk))) {
            FLAC__stream_encoder_finish(encoder);
            FLAC__stream_encoder_delete(encoder);
            throw std::runtime_error(
                QCoreApplication::translate("AudioIO", "FLAC encoding error writing: %1").arg(fileName).toStdString());
        }
    }

    FLAC__stream_encoder_finish(encoder);
    FLAC__stream_encoder_delete(encoder);

    return frameCount * channels;
}

// ---------- Public dispatch functions ----------

AudioFormat readAudioFormat(const QString &fileName)
{
    if (isFlacFileName(fileName)) {
        return readFlacFormat(fileName);
    }
    return readWavFormat(fileName);
}

ReadResultF32 readAudioFileF32(const QString &fileName)
{
    if (isFlacFileName(fileName)) {
        return readFlacFileF32(fileName);
    }
    return readWavFileF32(fileName);
}

ReadResultF64 readAudioFileF64(const QString &fileName)
{
    if (isFlacFileName(fileName)) {
        return readFlacFileF64(fileName);
    }
    return readWavFileF64(fileName);
}

size_t writeAudioFileF32(const QString &fileName, const kfr::univector2d<float> &data, const AudioFormat &targetFormat)
{
    if (targetFormat.isFlac()) {
        return writeFlacFileTyped(fileName, data, targetFormat);
    }
    return writeWavFileF32(fileName, data, targetFormat);
}

size_t writeAudioFileF64(const QString &fileName, const kfr::univector2d<double> &data, const AudioFormat &targetFormat)
{
    if (targetFormat.isFlac()) {
        return writeFlacFileTyped(fileName, data, targetFormat);
    }
    return writeWavFileF64(fileName, data, targetFormat);
}

size_t writeAudioFileF32(const QString &fileName, const kfr::univector2d<float> &data,
                         const kfr::audio_format &targetFormat)
{
    AudioFormat fmt;
    fmt.kfr_format = targetFormat;
    fmt.container = AudioFormat::Container::RIFF; // Default
    return writeAudioFileF32(fileName, data, fmt);
}

size_t writeAudioFileF64(const QString &fileName, const kfr::univector2d<double> &data,
                         const kfr::audio_format &targetFormat)
{
    AudioFormat fmt;
    fmt.kfr_format = targetFormat;
    fmt.container = AudioFormat::Container::RIFF; // Default
    return writeAudioFileF64(fileName, data, fmt);
}
// ---------- StreamingAudioWriter implementation ----------

// Platform compat wrapper for drwav_init_file_write (non-sequential — header patched on close)
#ifdef Q_OS_WIN
static drwav_bool32 drwav_init_file_write_compat(drwav *pWav, const QString &fileName,
                                                  const drwav_data_format *pFormat,
                                                  const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file_write_w(pWav, reinterpret_cast<const wchar_t *>(fileName.utf16()),
                                   pFormat, pAllocationCallbacks);
}
#else
static drwav_bool32 drwav_init_file_write_compat(drwav *pWav, const QString &fileName,
                                                  const drwav_data_format *pFormat,
                                                  const drwav_allocation_callbacks *pAllocationCallbacks)
{
    return drwav_init_file_write(pWav, fileName.toUtf8().constData(), pFormat, pAllocationCallbacks);
}
#endif

struct StreamingAudioWriter::WavState {
    drwav wav{};
    drwav_data_format format{};
};

struct StreamingAudioWriter::FlacState {
    FLAC__StreamEncoder *encoder = nullptr;
    unsigned bitsPerSample = 0;
    unsigned channels = 0;

    ~FlacState() {
        if (encoder) {
            FLAC__stream_encoder_delete(encoder);
            encoder = nullptr;
        }
    }
};

struct StreamingAudioWriter::DitherState {
    // KFR audio_quantization holds TPDF dither generator state
    kfr::audio_quantization quantization;
    explicit DitherState(unsigned bits) : quantization(bits, kfr::audio_dithering::triangular) {}
};

StreamingAudioWriter::StreamingAudioWriter() = default;

StreamingAudioWriter::~StreamingAudioWriter()
{
    if (m_isOpen)
        close();
}

StreamingAudioWriter::StreamingAudioWriter(StreamingAudioWriter &&other) noexcept
    : m_format(other.m_format),
      m_framesWritten(other.m_framesWritten),
      m_isOpen(other.m_isOpen),
      m_wavState(std::move(other.m_wavState)),
      m_isWav(other.m_isWav),
      m_flacState(std::move(other.m_flacState)),
      m_ditherState(std::move(other.m_ditherState))
{
    other.m_isOpen = false;
    other.m_framesWritten = 0;
}

StreamingAudioWriter &StreamingAudioWriter::operator=(StreamingAudioWriter &&other) noexcept
{
    if (this != &other) {
        if (m_isOpen)
            close();
        m_format = other.m_format;
        m_framesWritten = other.m_framesWritten;
        m_isOpen = other.m_isOpen;
        m_wavState = std::move(other.m_wavState);
        m_isWav = other.m_isWav;
        m_flacState = std::move(other.m_flacState);
        m_ditherState = std::move(other.m_ditherState);
        other.m_isOpen = false;
        other.m_framesWritten = 0;
    }
    return *this;
}

void StreamingAudioWriter::open(const QString &fileName, const AudioFormat &targetFormat,
                                qint64 totalFrameCountHint)
{
    if (m_isOpen)
        throw std::runtime_error("StreamingAudioWriter already open");

    m_format = targetFormat;
    m_framesWritten = 0;

    if (targetFormat.isFlac()) {
        // FLAC path
        auto state = std::make_unique<FlacState>();
        state->bitsPerSample = kfrTypeToFlacBits(targetFormat.kfr_format.type);
        state->channels = targetFormat.kfr_format.channels;

        state->encoder = FLAC__stream_encoder_new();
        if (!state->encoder)
            throw std::runtime_error(
                QCoreApplication::translate("AudioIO", "Failed to create FLAC encoder for: %1")
                    .arg(fileName).toStdString());

        FLAC__stream_encoder_set_channels(state->encoder, state->channels);
        FLAC__stream_encoder_set_bits_per_sample(state->encoder, state->bitsPerSample);
        FLAC__stream_encoder_set_sample_rate(state->encoder,
                                             static_cast<unsigned>(targetFormat.kfr_format.samplerate));
        FLAC__stream_encoder_set_compression_level(state->encoder, 5);
        if (totalFrameCountHint > 0)
            FLAC__stream_encoder_set_total_samples_estimate(state->encoder,
                                                            static_cast<FLAC__uint64>(totalFrameCountHint));

        QByteArray path = fileName.toUtf8();
        auto initStatus = FLAC__stream_encoder_init_file(state->encoder, path.constData(), nullptr, nullptr);
        if (initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
            throw std::runtime_error(
                QCoreApplication::translate("AudioIO", "Failed to open FLAC file for writing: %1")
                    .arg(fileName).toStdString());
        }

        m_flacState = std::move(state);
        m_isWav = false;

        // Set up dither state for FLAC (always integer output)
        m_ditherState = std::make_unique<DitherState>(m_flacState->bitsPerSample);
    } else {
        // WAV path (RIFF, RF64, W64)
        auto state = std::make_unique<WavState>();
        state->format = toDrWavDataFormat(targetFormat);

        if (!drwav_init_file_write_compat(&state->wav, fileName, &state->format, nullptr)) {
            throw std::runtime_error(
                QCoreApplication::translate("AudioIO", "Failed to open file for writing: %1")
                    .arg(fileName).toStdString());
        }

        m_wavState = std::move(state);
        m_isWav = true;

        // Set up dither state for PCM WAV output
        if (m_wavState->format.format == DR_WAVE_FORMAT_PCM) {
            m_ditherState = std::make_unique<DitherState>(m_wavState->format.bitsPerSample);
        }
    }

    m_isOpen = true;
}

void StreamingAudioWriter::writeFramesF32(const kfr::univector2d<float> &data)
{
    if (!m_isOpen || data.empty())
        return;

    if (m_isWav) {
        size_t frameCount = data[0].size();
        size_t channels = data.size();

        if (m_wavState->format.format == DR_WAVE_FORMAT_PCM) {
            // Need to promote to double for KFR's dithered quantization with persistent state
            auto dataDouble = promoteToDouble(data);
            auto ptrs = planarPointers(dataDouble);
            size_t totalSamples = frameCount * channels;
            auto &quant = m_ditherState->quantization;

            if (m_wavState->format.bitsPerSample == 16) {
                std::vector<kfr::i16> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            } else if (m_wavState->format.bitsPerSample == 24) {
                std::vector<kfr::i24> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            } else if (m_wavState->format.bitsPerSample == 32) {
                std::vector<kfr::i32> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            }
        } else if (m_wavState->format.format == DR_WAVE_FORMAT_IEEE_FLOAT &&
                   m_wavState->format.bitsPerSample == 32) {
            std::vector<float> interleaved;
            interleaveFrom(interleaved, data);
            drwav_write_pcm_frames(&m_wavState->wav, frameCount, interleaved.data());
        } else if (m_wavState->format.format == DR_WAVE_FORMAT_IEEE_FLOAT &&
                   m_wavState->format.bitsPerSample == 64) {
            auto dataDouble = promoteToDouble(data);
            std::vector<double> interleaved;
            interleaveFrom(interleaved, dataDouble);
            drwav_write_pcm_frames(&m_wavState->wav, frameCount, interleaved.data());
        }
    } else {
        // FLAC path
        size_t frameCount = data[0].size();
        size_t channels = data.size();
        unsigned bps = m_flacState->bitsPerSample;
        double scale = static_cast<double>(1ULL << (bps - 1));
        double maxVal = scale - 1.0;
        double minVal = -scale;
        auto &quant = m_ditherState->quantization;

        constexpr size_t chunkFrames = 4096;
        std::vector<FLAC__int32> buffer(chunkFrames * channels);

        for (size_t offset = 0; offset < frameCount; offset += chunkFrames) {
            size_t thisChunk = std::min(chunkFrames, frameCount - offset);
            for (size_t i = 0; i < thisChunk; ++i) {
                for (size_t c = 0; c < channels; ++c) {
                    double sample = static_cast<double>(data[c][offset + i]);
                    double dithered = std::clamp(sample + quant.dither(), -1.0, 1.0);
                    double scaled = dithered * scale;
                    scaled = std::clamp(scaled, minVal, maxVal);
                    buffer[i * channels + c] = static_cast<FLAC__int32>(std::llround(scaled));
                }
            }
            if (!FLAC__stream_encoder_process_interleaved(m_flacState->encoder, buffer.data(),
                                                          static_cast<unsigned>(thisChunk))) {
                throw std::runtime_error("FLAC streaming encode error");
            }
        }
    }

    m_framesWritten += static_cast<qint64>(data[0].size());
}

void StreamingAudioWriter::writeFramesF64(const kfr::univector2d<double> &data)
{
    if (!m_isOpen || data.empty())
        return;

    if (m_isWav) {
        size_t frameCount = data[0].size();
        size_t channels = data.size();

        if (m_wavState->format.format == DR_WAVE_FORMAT_PCM) {
            auto ptrs = planarPointers(data);
            size_t totalSamples = frameCount * channels;
            auto &quant = m_ditherState->quantization;

            if (m_wavState->format.bitsPerSample == 16) {
                std::vector<kfr::i16> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            } else if (m_wavState->format.bitsPerSample == 24) {
                std::vector<kfr::i24> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            } else if (m_wavState->format.bitsPerSample == 32) {
                std::vector<kfr::i32> buf(totalSamples);
                kfr::samples_store(buf.data(), ptrs.data(), channels, frameCount, quant);
                drwav_write_pcm_frames(&m_wavState->wav, frameCount, buf.data());
            }
        } else if (m_wavState->format.format == DR_WAVE_FORMAT_IEEE_FLOAT &&
                   m_wavState->format.bitsPerSample == 64) {
            std::vector<double> interleaved;
            interleaveFrom(interleaved, data);
            drwav_write_pcm_frames(&m_wavState->wav, frameCount, interleaved.data());
        } else if (m_wavState->format.format == DR_WAVE_FORMAT_IEEE_FLOAT &&
                   m_wavState->format.bitsPerSample == 32) {
            // double -> float
            std::vector<float> interleavedF32(frameCount * channels);
            for (size_t i = 0; i < frameCount; ++i) {
                for (size_t c = 0; c < channels; ++c) {
                    double v = (c < data.size() && i < data[c].size()) ? data[c][i] : 0.0;
                    interleavedF32[i * channels + c] = static_cast<float>(v);
                }
            }
            drwav_write_pcm_frames(&m_wavState->wav, frameCount, interleavedF32.data());
        }
    } else {
        // FLAC path
        size_t frameCount = data[0].size();
        size_t channels = data.size();
        unsigned bps = m_flacState->bitsPerSample;
        double scale = static_cast<double>(1ULL << (bps - 1));
        double maxVal = scale - 1.0;
        double minVal = -scale;
        auto &quant = m_ditherState->quantization;

        constexpr size_t chunkFrames = 4096;
        std::vector<FLAC__int32> buffer(chunkFrames * channels);

        for (size_t offset = 0; offset < frameCount; offset += chunkFrames) {
            size_t thisChunk = std::min(chunkFrames, frameCount - offset);
            for (size_t i = 0; i < thisChunk; ++i) {
                for (size_t c = 0; c < channels; ++c) {
                    double sample = data[c][offset + i];
                    double dithered = std::clamp(sample + quant.dither(), -1.0, 1.0);
                    double scaled = dithered * scale;
                    scaled = std::clamp(scaled, minVal, maxVal);
                    buffer[i * channels + c] = static_cast<FLAC__int32>(std::llround(scaled));
                }
            }
            if (!FLAC__stream_encoder_process_interleaved(m_flacState->encoder, buffer.data(),
                                                          static_cast<unsigned>(thisChunk))) {
                throw std::runtime_error("FLAC streaming encode error");
            }
        }
    }

    m_framesWritten += static_cast<qint64>(data[0].size());
}

void StreamingAudioWriter::writeSilence(qint64 frameCount)
{
    if (!m_isOpen || frameCount <= 0)
        return;

    size_t channels = m_format.kfr_format.channels;
    constexpr qint64 chunkSize = 4096;

    // Allocate a small zero-filled buffer and loop
    kfr::univector2d<float> silenceBuf(channels);
    for (size_t c = 0; c < channels; ++c)
        silenceBuf[c].resize(static_cast<size_t>(std::min(frameCount, chunkSize)), 0.0f);

    qint64 remaining = frameCount;
    while (remaining > 0) {
        qint64 thisChunk = std::min(remaining, chunkSize);
        if (thisChunk < chunkSize) {
            for (size_t c = 0; c < channels; ++c)
                silenceBuf[c].resize(static_cast<size_t>(thisChunk));
        }
        writeFramesF32(silenceBuf);
        // Correct framesWritten — writeFramesF32 already increments it,
        // but the increment was based on data[0].size() which may differ
        // from what we want for the last chunk.
        remaining -= thisChunk;
    }
}

void StreamingAudioWriter::close()
{
    if (!m_isOpen)
        return;

    if (m_isWav && m_wavState) {
        drwav_uninit(&m_wavState->wav);
        m_wavState.reset();
    }

    if (m_flacState) {
        if (m_flacState->encoder) {
            FLAC__stream_encoder_finish(m_flacState->encoder);
        }
        m_flacState.reset();
    }

    m_ditherState.reset();
    m_isOpen = false;
}

} // namespace AudioIO
