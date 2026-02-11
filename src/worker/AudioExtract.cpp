#include "AudioExtract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <kfr/dsp/dcremove.hpp>
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <mutex>
#include <type_traits>
#include <variant>

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/parallel_pipeline.h>
#pragma pop_macro("emit")

#include "AudioIO.h"

using namespace utils;

namespace AudioExtract {

CheckResult preCheck(QString srcWAVFileName, QString dstDirName)
{
    // Check Desc file
    auto descFileName = getDescFileNameFrom(srcWAVFileName);
    QFile descFile(descFileName);

    if (!descFile.exists()) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate("WAVExtract",
                                            "<p class='critical'>Can not find description file \"%1\". "
                                            "If you have renamed the WAV file, please also rename the desc file.</p>")
                    .arg(descFileName),
                {}};
    }

    if (!descFile.open(QIODevice::ReadOnly)) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate("WAVExtract", "<p class='critical'>Can not open description file \"%1\". "
                                                          "Check your permission.</p>")
                    .arg(descFileName),
                {}};
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(descFile.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate("WAVExtract",
                                            "<p class='critical'>Failed to parse description file \"%1\": %2</p>")
                    .arg(descFileName)
                    .arg(err.errorString()),
                {}};
    }

    auto root = doc.object();

    // Validate version
    if (!root.contains("version")) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate("WAVExtract",
                                            "<p class='critical'>Description file \"%1\" is missing version field.</p>")
                    .arg(descFileName),
                {}};
    }

    int version = root["version"].toInt();
    if (version < utils::min_supported_desc_file_version || version > utils::desc_file_version) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate("WAVExtract",
                                            "<p class='critical'>Description file \"%1\" has unsupported "
                                            "version %2. Expected version %3 to %4.</p>")
                    .arg(descFileName)
                    .arg(version)
                    .arg(utils::min_supported_desc_file_version)
                    .arg(utils::desc_file_version),
                {}};
    }

    // Validate all volume files exist for multi-volume
    {
        int volumeCount = root["volume_count"].toInt(1);
        if (volumeCount > 1) {
            QString missingFiles;
            for (int i = 0; i < volumeCount; ++i) {
                QString volFile = utils::getVolumeFileName(srcWAVFileName, i);
                if (!QFile::exists(volFile)) {
                    missingFiles +=
                        QCoreApplication::translate("WAVExtract", "<p class='critical'>Missing volume file: \"%1\"</p>")
                            .arg(volFile);
                }
            }
            if (!missingFiles.isEmpty()) {
                return {CheckPassType::CRITICAL, missingFiles, {}};
            }
        }
    }

    return {CheckPassType::OK, "", root};
}

// --- Pipeline token ---

struct ExtractToken
{
    utils::AudioBufferPtr audio;
    QJsonObject descObj;
    double srcSampleRate;
    bool useDouble;
    QString errorDescription; // non-empty if read failed
};

// --- Helper: process and write a single extracted entry ---

template <typename T>
static void processAndWriteEntry(kfr::univector2d<T> &slice, const QJsonObject &descObj,
                                 double srcSampleRate, const QString &dstDirName,
                                 const std::optional<AudioIO::AudioFormat> &targetFormat,
                                 bool removeDCOffset)
{
    if (removeDCOffset) {
        for (size_t c = 0; c < slice.size(); ++c) {
            slice[c] = kfr::dcremove(slice[c], 10.0, srcSampleRate);
        }
    }

    // Determine Output Format
    AudioIO::AudioFormat outputFormat;
    QString relativePath = descObj["file_name"].toString();

    if (!targetFormat.has_value()) {
        // "Same as source when combining" — inherit everything from description
        outputFormat.kfr_format.samplerate = descObj["sample_rate"].toDouble();
        outputFormat.kfr_format.type = (kfr::audio_sample_type)descObj["sample_type"].toInt();
        outputFormat.kfr_format.channels = descObj["channel_count"].toInt();
        int cfmt = descObj.contains("container_format") ? descObj["container_format"].toInt()
                                                        : descObj["wav_format"].toInt();
        switch (cfmt) {
        case 0:
            outputFormat.container = AudioIO::AudioFormat::Container::RIFF;
            break;
        case 1:
            outputFormat.container = AudioIO::AudioFormat::Container::W64;
            break;
        case 2:
            outputFormat.container = AudioIO::AudioFormat::Container::RF64;
            break;
        case 3:
            outputFormat.container = AudioIO::AudioFormat::Container::FLAC;
            break;
        default:
            outputFormat.container = AudioIO::AudioFormat::Container::RIFF;
            break;
        }
    } else {
        const auto &fmt = *targetFormat;
        outputFormat.kfr_format.samplerate =
            (fmt.kfr_format.samplerate < 1e-5) ? descObj["sample_rate"].toDouble() : fmt.kfr_format.samplerate;
        outputFormat.kfr_format.type = (fmt.kfr_format.type == kfr::audio_sample_type::unknown)
                                           ? (kfr::audio_sample_type)descObj["sample_type"].toInt()
                                           : fmt.kfr_format.type;
        outputFormat.kfr_format.channels =
            (fmt.kfr_format.channels == 0) ? descObj["channel_count"].toInt() : fmt.kfr_format.channels;
        outputFormat.container = fmt.container;
    }

    // Resample if needed
    double targetRate = outputFormat.kfr_format.samplerate;
    if (std::abs(srcSampleRate - targetRate) > 1e-5) {
        kfr::univector2d<T> resampledSlice(slice.size());
        for (size_t c = 0; c < slice.size(); ++c) {
            auto chConverter = kfr::sample_rate_converter<T>(
                utils::getSampleRateConversionQuality(), (size_t)targetRate, (size_t)srcSampleRate);
            size_t newSize = chConverter.output_size_for_input(slice[c].size());
            resampledSlice[c].resize(newSize);
            chConverter.process(resampledSlice[c], slice[c]);
        }
        slice = std::move(resampledSlice);
    }

    // Channel mix
    size_t outChannels = outputFormat.kfr_format.channels;
    kfr::univector2d<T> finalData(outChannels);
    for (size_t c = 0; c < outChannels; ++c) {
        if (c < slice.size())
            finalData[c] = slice[c];
        else
            finalData[c].resize(slice[0].size(), T(0));
    }

    // Write File
    QString outFileName = dstDirName + "/" + relativePath;
    if (outputFormat.isFlac() && !outFileName.endsWith(".flac", Qt::CaseInsensitive)) {
        outFileName = QFileInfo(outFileName).path() + "/" + QFileInfo(outFileName).completeBaseName() + ".flac";
    } else if (!outputFormat.isFlac() && !outFileName.endsWith(".wav", Qt::CaseInsensitive)) {
        outFileName = QFileInfo(outFileName).path() + "/" + QFileInfo(outFileName).completeBaseName() + ".wav";
    }
    QDir().mkpath(QFileInfo(outFileName).absolutePath());

    if (utils::shouldUseDoubleInternalProcessing(outputFormat.kfr_format.type)) {
        if constexpr (std::is_same_v<T, double>) {
            AudioIO::writeAudioFileF64(outFileName, finalData, outputFormat);
        } else {
            kfr::univector2d<double> promoted(finalData.size());
            for (size_t c = 0; c < finalData.size(); ++c) {
                promoted[c].resize(finalData[c].size());
                for (size_t i = 0; i < finalData[c].size(); ++i)
                    promoted[c][i] = finalData[c][i];
            }
            AudioIO::writeAudioFileF64(outFileName, promoted, outputFormat);
        }
    } else {
        if constexpr (std::is_same_v<T, float>) {
            AudioIO::writeAudioFileF32(outFileName, finalData, outputFormat);
        } else {
            kfr::univector2d<float> downcast(finalData.size());
            for (size_t c = 0; c < finalData.size(); ++c) {
                downcast[c].resize(finalData[c].size());
                for (size_t i = 0; i < finalData[c].size(); ++i)
                    downcast[c][i] = static_cast<float>(finalData[c][i]);
            }
            AudioIO::writeAudioFileF32(outFileName, downcast, outputFormat);
        }
    }
}

ExtractPipelineResult runExtractPipeline(const ExtractPipelineParams &params,
                                          std::atomic<int> &progress,
                                          oneapi::tbb::task_group_context &ctx)
{
    const QJsonObject &descRoot = params.descRoot;
    double sampleRate = descRoot["sample_rate"].toDouble();
    int volumeCount = descRoot["volume_count"].toInt(1);

    // Decide internal processing precision
    bool useDouble = false;
    if (params.targetFormat.has_value() &&
        params.targetFormat->kfr_format.type != kfr::audio_sample_type::unknown) {
        useDouble = utils::shouldUseDoubleInternalProcessing(params.targetFormat->kfr_format.type);
    } else {
        auto srcFormat = AudioIO::readAudioFormat(params.srcWAVFileName);
        useDouble = utils::shouldUseDoubleInternalProcessing(srcFormat.kfr_format.type);
    }

    // Build sorted entry list: sort by (volume_index, begin_time) for sequential disk access
    struct EntryInfo {
        QJsonObject descObj;
        int volumeIndex;
        qint64 beginSample;
        int originalIndex;
    };

    QList<EntryInfo> entries;
    const QJsonArray &descArray = params.filteredDescArray;
    entries.reserve(descArray.size());
    for (int i = 0; i < descArray.size(); ++i) {
        QJsonObject obj = descArray[i].toObject();
        int volIdx = obj["volume_index"].toInt(0);
        qint64 beginSample = timecodeToSamples(obj["begin_time"].toString(), sampleRate);
        entries.append({obj, volIdx, beginSample, i});
    }
    std::sort(entries.begin(), entries.end(), [](const EntryInfo &a, const EntryInfo &b) {
        if (a.volumeIndex != b.volumeIndex)
            return a.volumeIndex < b.volumeIndex;
        return a.beginSample < b.beginSample;
    });

    // Compute gap samples
    const qint64 gapSamples =
        (params.gapMode == ExtractGapMode::IncludeSpace && !params.gapDurationTimecode.isEmpty())
            ? timecodeToSamples(params.gapDurationTimecode, sampleRate)
            : 0;

    // Thread-safe error accumulation
    std::mutex errorMutex;
    QList<ExtractErrorDescription> errors;

    int totalEntries = entries.size();
    int nextIndex = 0;

    // Streaming reader — owned by the serial stage
    AudioIO::StreamingAudioReader reader;
    int currentReaderVolume = -1;

    auto filterChain =
        // Stage 1: serial_in_order — seek + read from source
        tbb::make_filter<void, ExtractToken>(
            tbb::filter_mode::serial_in_order,
            [&](tbb::flow_control &fc) -> ExtractToken {
                if (nextIndex >= totalEntries) {
                    fc.stop();
                    return {};
                }

                const auto &entry = entries[nextIndex++];

                ExtractToken token;
                token.descObj = entry.descObj;
                token.srcSampleRate = sampleRate;
                token.useDouble = useDouble;

                try {
                    // Handle volume switching
                    int entryVolume = entry.volumeIndex;
                    if (entryVolume != currentReaderVolume) {
                        if (reader.isOpen())
                            reader.close();

                        QString srcFile;
                        if (volumeCount <= 1) {
                            srcFile = params.srcWAVFileName;
                        } else {
                            srcFile = utils::getVolumeFileName(params.srcWAVFileName, entryVolume);
                        }
                        reader.open(srcFile);
                        currentReaderVolume = entryVolume;
                    }

                    // Calculate seek position and read range
                    qint64 startSample = entry.beginSample;
                    qint64 durSample = timecodeToSamples(entry.descObj["duration"].toString(), sampleRate);

                    // Adjust range if IncludeSpace mode
                    if (params.gapMode == ExtractGapMode::IncludeSpace && gapSamples > 0) {
                        qint64 adjustedStart = (startSample > gapSamples) ? startSample - gapSamples : 0;
                        qint64 originalEnd = startSample + durSample;
                        startSample = adjustedStart;
                        durSample = originalEnd + gapSamples - adjustedStart;
                    }

                    // Clamp to file length
                    qint64 fileLength = reader.format().length;
                    if (startSample >= fileLength) {
                        throw std::runtime_error("Start time out of range");
                    }
                    if (startSample + durSample > fileLength) {
                        durSample = fileLength - startSample;
                    }

                    // Seek and read
                    if (!reader.seekToFrame(startSample)) {
                        throw std::runtime_error(
                            QCoreApplication::translate("WAVExtract", "Failed to seek in source file")
                                .toStdString());
                    }

                    if (useDouble) {
                        auto data = reader.readFramesF64(durSample);
                        token.audio = std::make_shared<AudioBufferF64>(std::move(data));
                    } else {
                        auto data = reader.readFramesF32(durSample);
                        token.audio = std::make_shared<AudioBufferF32>(std::move(data));
                    }
                } catch (const std::exception &e) {
                    token.errorDescription = QString::fromStdString(e.what());
                } catch (...) {
                    token.errorDescription = QCoreApplication::translate("WAVExtract", "Unknown Error");
                }

                return token;
            }) &
        // Stage 2: parallel — process + write
        tbb::make_filter<ExtractToken, void>(
            tbb::filter_mode::parallel,
            [&](ExtractToken token) {
                // Skip entries that failed to read
                if (!token.errorDescription.isEmpty()) {
                    std::lock_guard lock(errorMutex);
                    errors.append({token.errorDescription, token.descObj});
                    progress.fetch_add(1, std::memory_order_relaxed);
                    return;
                }

                try {
                    std::visit(
                        [&](const auto &ptr) {
                            if (!ptr)
                                throw std::runtime_error("Null audio buffer in extract pipeline");

                            using PtrT = std::decay_t<decltype(ptr)>;
                            using T = std::conditional_t<
                                std::is_same_v<PtrT, std::shared_ptr<utils::AudioBufferF64>>, double, float>;

                            // Make a mutable copy for processing
                            kfr::univector2d<T> slice = *ptr;
                            processAndWriteEntry(slice, token.descObj, token.srcSampleRate,
                                                 params.dstDirName, params.targetFormat,
                                                 params.removeDCOffset);
                        },
                        token.audio);
                } catch (const std::exception &e) {
                    std::lock_guard lock(errorMutex);
                    errors.append({QString::fromStdString(e.what()), token.descObj});
                } catch (...) {
                    std::lock_guard lock(errorMutex);
                    errors.append(
                        {QCoreApplication::translate("WAVExtract", "Unknown Error"), token.descObj});
                }

                progress.fetch_add(1, std::memory_order_relaxed);
            });

    {
        const tbb::filter<void, void> &chain = filterChain;
        tbb::parallel_pipeline(static_cast<size_t>(utils::effectiveMaxLiveTokens()), chain, ctx);
    }

    if (reader.isOpen())
        reader.close();

    return {errors};
}

} // namespace AudioExtract
