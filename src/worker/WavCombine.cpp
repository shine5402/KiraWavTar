#include "WavCombine.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrent>
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <memory>
#include <type_traits>
#include <variant>

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/parallel_pipeline.h>
#pragma pop_macro("emit")

#include "AudioIO.h"
#include "utils/Filesystem.h"
#include "utils/KfrHelper.h"
#include "utils/Utils.h"

using namespace utils;

namespace AudioCombine {

// --- preCheck (unchanged) ---

CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, AudioIO::AudioFormat targetFormat)
{

    auto wavFileNames = getAbsoluteAudioFileNamesUnder(rootDirName, recursive);

    if (wavFileNames.isEmpty()) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate(
                    "WAVCombine", "<p class='critical'>There are not any audio files in the given folder. "
                                  "Please check the path, or if you forget to turn \"scan subfolders\" on?</p>"),
                wavFileNames};
    }

    // As mapped() need result_type member to work, so we use std::function() to help us.
    auto formats = QtConcurrent::mapped(
                       wavFileNames, std::function([](const QString &fileName) -> QPair<QString, AudioIO::AudioFormat> {
                           try {
                               auto format = AudioIO::readAudioFormat(fileName);
                               return {fileName, format};
                           } catch (...) {
                               return {fileName, AudioIO::AudioFormat{}};
                           }
                       }))
                       .results();

    qint64 totalLength = 0;
    for (const auto &i : formats) {
        totalLength += i.second.length;
    }

    if (totalLength > INT32_MAX && targetFormat.container == AudioIO::AudioFormat::Container::RIFF) {
        return {
            CheckPassType::CRITICAL,
            QCoreApplication::translate(
                "WAVCombine",
                "<p class='critical'>Length of the wav file combined will be too large to save in normal RIFF WAVs. "
                "Please use 64-bit WAV instead.</p>"),
            wavFileNames};
    }

    QString warningMsg;

    auto hasUnknownType = QtConcurrent::filtered(formats, [](const QPair<QString, AudioIO::AudioFormat> &info) -> bool {
                              return info.second.kfr_format.type == kfr::audio_sample_type::unknown;
                          }).results();

    for (const auto &i : std::as_const(hasUnknownType)) {
        warningMsg.append(QCoreApplication::translate("WAVCombine",
                                                      "<p class='warning'>Can not know bit depth from \"%1\"."
                                                      " Maybe this file id corrupted, "
                                                      "or error happened during opening the file.</p>")
                              .arg(i.first));
    }

    auto hasMultipleChannels =
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::AudioFormat> &info) -> bool {
            return info.second.kfr_format.channels > targetFormat.kfr_format.channels;
        }).results();

    for (const auto &i : std::as_const(hasMultipleChannels)) {
        warningMsg.append(QCoreApplication::translate("WAVCombine",
                                                      "<p class='warning'>There are %2 channel(s) in \"%1\". "
                                                      "Channels after No.%3 will be discarded.</p>")
                              .arg(i.first)
                              .arg(i.second.kfr_format.channels)
                              .arg(targetFormat.kfr_format.channels));
    }

    auto hasLargerSampleRate =
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::AudioFormat> &info) -> bool {
            return info.second.kfr_format.samplerate > targetFormat.kfr_format.samplerate;
        }).results();

    for (const auto &i : std::as_const(hasLargerSampleRate)) {
        warningMsg.append(QCoreApplication::translate(
                              "WAVCombine",
                              "<p class='warning'>Sample rate (%2 Hz) of \"%1\" is larger than target (\"%3\" Hz)."
                              "The precision could be lost a bit when processing.</p>")
                              .arg(i.first)
                              .arg(i.second.kfr_format.samplerate)
                              .arg(targetFormat.kfr_format.samplerate));
    }

    // --- Sample type conversion warnings ---
    for (const auto &[fileName, fileFormat] : std::as_const(formats)) {
        auto srcType = fileFormat.kfr_format.type;
        auto dstType = targetFormat.kfr_format.type;

        if (srcType == dstType || srcType == kfr::audio_sample_type::unknown)
            continue;

        bool srcIsFloat = kfr::audio_sample_is_float(srcType);
        bool dstIsFloat = kfr::audio_sample_is_float(dstType);

        if (!srcIsFloat && dstIsFloat) {
            continue;
        }

        if (srcIsFloat && !dstIsFloat) {
            warningMsg.append(
                QCoreApplication::translate(
                    "WAVCombine",
                    "<p class='warning'>\"%1\" (%2) will be converted to integer format (%3). "
                    "Floating-point audio has virtually unlimited headroom, but integer formats clip at 0 dBFS "
                    "\u2014 any signal above that will be permanently burned in. Quiet signals may also lose detail "
                    "due to the reduced dynamic range. "
                    "TPDF dithering is applied automatically to reduce quantization artifacts.</p>")
                    .arg(fileName)
                    .arg(kfr::audio_sample_type_to_string(srcType).data())
                    .arg(kfr::audio_sample_type_to_string(dstType).data()));
        } else if (srcIsFloat && dstIsFloat) {
            if (kfr::audio_sample_type_to_precision(srcType) > kfr::audio_sample_type_to_precision(dstType)) {
                warningMsg.append(QCoreApplication::translate(
                                      "WAVCombine", "<p class='warning'>\"%1\" (%2) will be converted to %3. "
                                                    "There will be a slight reduction in floating-point precision, "
                                                    "which is generally negligible for most audio work.</p>")
                                      .arg(fileName)
                                      .arg(kfr::audio_sample_type_to_string(srcType).data())
                                      .arg(kfr::audio_sample_type_to_string(dstType).data()));
            }
        } else {
            int srcBits = kfr::audio_sample_type_to_precision(srcType);
            int dstBits = kfr::audio_sample_type_to_precision(dstType);
            if (srcBits > dstBits) {
                warningMsg.append(QCoreApplication::translate(
                                      "WAVCombine",
                                      "<p class='warning'>\"%1\" (%2) will be converted to %3. "
                                      "This reduces the dynamic range, which means quiet signals may lose detail. "
                                      "TPDF dithering is applied automatically to reduce quantization artifacts.</p>")
                                      .arg(fileName)
                                      .arg(kfr::audio_sample_type_to_string(srcType).data())
                                      .arg(kfr::audio_sample_type_to_string(dstType).data()));
            }
        }
    }

    if (warningMsg.isEmpty())
        return {CheckPassType::OK, "", wavFileNames};

    return {CheckPassType::WARNING, warningMsg, wavFileNames};
}

// --- Helper: container format to integer for JSON ---

static int containerFormatToInt(AudioIO::AudioFormat::Container c)
{
    switch (c) {
    case AudioIO::AudioFormat::Container::RIFF:
        return 0;
    case AudioIO::AudioFormat::Container::W64:
        return 1;
    case AudioIO::AudioFormat::Container::RF64:
        return 2;
    case AudioIO::AudioFormat::Container::FLAC:
        return 3;
    default:
        return 0;
    }
}

// --- computeLayout ---

CombineLayout computeLayout(const QStringList &wavFileNames, const QString &rootDirName, const QString &saveFileName,
                            const AudioIO::AudioFormat &targetFormat, int gapMs,
                            const utils::VolumeConfig &volumeConfig)
{
    CombineLayout layout;
    layout.targetFormat = targetFormat;
    layout.gapSamples = qRound64(gapMs / 1000.0 * targetFormat.kfr_format.samplerate);
    layout.useDouble = utils::shouldUseDoubleInternalProcessing(targetFormat.kfr_format.type);
    layout.rootDirName = rootDirName;

    QDir rootDir(rootDirName);

    // Read formats for all files (reuse cached info from preCheck where possible)
    layout.entries.reserve(wavFileNames.size());
    for (const auto &fileName : wavFileNames) {
        EntryLayout entry;
        entry.filePath = fileName;
        entry.relativePath = rootDir.relativeFilePath(fileName);
        try {
            entry.sourceFormat = AudioIO::readAudioFormat(fileName);
        } catch (...) {
            entry.sourceFormat = {};
        }
        layout.entries.append(entry);
    }

    // Estimate per-entry output frame counts
    auto estimateOutputFrames = [&](const EntryLayout &entry) -> qint64 {
        if (entry.sourceFormat.length <= 0)
            return 0;
        double inputRate = entry.sourceFormat.kfr_format.samplerate;
        double outputRate = targetFormat.kfr_format.samplerate;
        if (std::abs(inputRate - outputRate) > 1e-5 && inputRate > 0) {
            return static_cast<qint64>(
                std::ceil(static_cast<double>(entry.sourceFormat.length) * outputRate / inputRate));
        }
        return entry.sourceFormat.length;
    };

    if (volumeConfig.mode == utils::VolumeSplitMode::None) {
        // Single volume
        VolumeLayout vol;
        vol.outputFilePath = saveFileName;
        qint64 totalFrames = 0;
        for (int i = 0; i < layout.entries.size(); ++i) {
            layout.entries[i].volumeIndex = 0;
            vol.entryIndices.append(i);
            qint64 entryFrames = estimateOutputFrames(layout.entries[i]);
            totalFrames += entryFrames + 2 * layout.gapSamples; // leading + trailing gap
        }
        vol.totalFrames = totalFrames;
        layout.volumes.append(vol);
    } else if (volumeConfig.mode == utils::VolumeSplitMode::ByCount) {
        int maxPerVol = volumeConfig.maxEntriesPerVolume;
        int volIndex = 0;
        for (int i = 0; i < layout.entries.size(); i += maxPerVol) {
            VolumeLayout vol;
            vol.outputFilePath = getVolumeFileName(saveFileName, volIndex);
            qint64 totalFrames = 0;
            int end = std::min(i + maxPerVol, static_cast<int>(layout.entries.size()));
            for (int j = i; j < end; ++j) {
                layout.entries[j].volumeIndex = volIndex;
                vol.entryIndices.append(j);
                totalFrames += estimateOutputFrames(layout.entries[j]) + 2 * layout.gapSamples;
            }
            vol.totalFrames = totalFrames;
            layout.volumes.append(vol);
            ++volIndex;
        }
    } else {
        // ByDuration
        double sampleRate = targetFormat.kfr_format.samplerate;
        qint64 maxDurationSamples =
            static_cast<qint64>(volumeConfig.maxDurationSeconds) * static_cast<qint64>(sampleRate);

        int volIndex = 0;
        VolumeLayout currentVol;
        currentVol.outputFilePath = getVolumeFileName(saveFileName, volIndex);
        qint64 accumulatedSamples = 0;

        for (int i = 0; i < layout.entries.size(); ++i) {
            qint64 entryFrames = estimateOutputFrames(layout.entries[i]);
            qint64 footprint = 2 * layout.gapSamples + entryFrames;

            if (footprint > maxDurationSamples) {
                throw std::runtime_error(
                    QCoreApplication::translate(
                        "WAVCombine",
                        "Entry \"%1\" has duration which exceeds the maximum volume duration of %2 seconds.")
                        .arg(layout.entries[i].relativePath)
                        .arg(volumeConfig.maxDurationSeconds)
                        .toStdString());
            }

            if (accumulatedSamples > 0 && accumulatedSamples + footprint > maxDurationSamples) {
                currentVol.totalFrames = accumulatedSamples;
                layout.volumes.append(currentVol);
                ++volIndex;
                currentVol = {};
                currentVol.outputFilePath = getVolumeFileName(saveFileName, volIndex);
                accumulatedSamples = 0;
            }

            layout.entries[i].volumeIndex = volIndex;
            currentVol.entryIndices.append(i);
            accumulatedSamples += footprint;
        }
        currentVol.totalFrames = accumulatedSamples;
        layout.volumes.append(currentVol);
    }

    return layout;
}

// --- Pipeline token ---

struct CombineToken
{
    utils::AudioBufferPtr processedAudio;
    QJsonObject entryMeta;
    int entryIndex;
};

// --- runCombinePipeline ---

QJsonObject runCombinePipeline(const CombineLayout &layout, std::atomic<int> &progress,
                               oneapi::tbb::task_group_context &ctx)
{
    // Open all volume writers (non-copyable, so use vector of unique_ptr)
    std::vector<std::unique_ptr<AudioIO::StreamingAudioWriter>> writers;
    writers.reserve(layout.volumes.size());
    for (int v = 0; v < layout.volumes.size(); ++v) {
        auto w = std::make_unique<AudioIO::StreamingAudioWriter>();
        w->open(layout.volumes[v].outputFilePath, layout.targetFormat, layout.volumes[v].totalFrames);
        writers.push_back(std::move(w));
    }

    int currentVolume = 0;
    QJsonArray descriptionsArray;
    qint64 currentSamplePos = 0;
    bool isFirstEntryInVolume = true;

    int totalEntries = layout.entries.size();
    int nextIndex = 0;

    auto filterChain =
        // Stage 1: serial_in_order — emit entry indices
        tbb::make_filter<void, int>(tbb::filter_mode::serial_in_order,
                                    [&nextIndex, totalEntries](tbb::flow_control &fc) -> int {
                                        if (nextIndex >= totalEntries) {
                                            fc.stop();
                                            return -1;
                                        }
                                        return nextIndex++;
                                    }) &
        // Stage 2: parallel — read file, resample, channel mix
        tbb::make_filter<int, CombineToken>(
            tbb::filter_mode::parallel,
            [&layout](int entryIndex) -> CombineToken {
                const auto &entry = layout.entries[entryIndex];
                const auto &targetFormat = layout.targetFormat;

                auto processTyped = [&](auto &&readResult) -> CombineToken {
                    using T = std::decay_t<decltype(readResult.data[0][0])>;

                    auto &inputData = readResult.data;
                    size_t inputChannels = readResult.format.kfr_format.channels;
                    size_t inputLength = readResult.format.length;
                    double inputSampleRate = readResult.format.kfr_format.samplerate;

                    size_t outputChannels = targetFormat.kfr_format.channels;
                    double outputSampleRate = targetFormat.kfr_format.samplerate;

                    // Channel mix (truncate extra or pad with zeros)
                    kfr::univector2d<T> processedData(outputChannels);
                    for (size_t c = 0; c < outputChannels; ++c) {
                        if (c < inputChannels) {
                            processedData[c] = inputData[c];
                        } else {
                            processedData[c].resize(inputLength, T(0));
                        }
                    }

                    // Resample if needed
                    if (std::abs(inputSampleRate - outputSampleRate) > 1e-5) {
                        kfr::univector2d<T> resampledData(outputChannels);
                        for (size_t c = 0; c < outputChannels; ++c) {
                            if (c >= processedData.size())
                                continue;
                            auto converter =
                                kfr::sample_rate_converter<T>(utils::getSampleRateConversionQuality(),
                                                              (size_t)outputSampleRate, (size_t)inputSampleRate);
                            size_t outSize = converter.output_size_for_input(processedData[c].size());
                            resampledData[c].resize(outSize);
                            converter.process(resampledData[c], processedData[c]);
                        }
                        processedData = std::move(resampledData);
                    }

                    // Build per-entry metadata
                    QJsonObject metaObj;
                    metaObj.insert("file_name", entry.relativePath);
                    metaObj.insert("sample_rate", readResult.format.kfr_format.samplerate);
                    metaObj.insert("sample_type", (qint64)readResult.format.kfr_format.type);
                    metaObj.insert("container_format", containerFormatToInt(readResult.format.container));
                    metaObj.insert("channel_count", (qint64)readResult.format.kfr_format.channels);
                    metaObj.insert("duration", samplesToTimecode(readResult.format.length,
                                                                 readResult.format.kfr_format.samplerate));

                    auto retData = std::make_shared<kfr::univector2d<T>>(std::move(processedData));
                    utils::AudioBufferPtr buf;
                    if constexpr (std::is_same_v<T, float>)
                        buf = retData;
                    else
                        buf = retData;

                    return CombineToken{buf, metaObj, entryIndex};
                };

                if (layout.useDouble) {
                    auto result = AudioIO::readAudioFileF64(entry.filePath);
                    return processTyped(std::move(result));
                }
                auto result = AudioIO::readAudioFileF32(entry.filePath);
                return processTyped(std::move(result));
            }) &
        // Stage 3: serial_in_order — write sequentially to output
        tbb::make_filter<CombineToken, void>(tbb::filter_mode::serial_in_order, [&](CombineToken token) {
            const auto &entry = layout.entries[token.entryIndex];

            // Check if we need to advance to next volume
            if (entry.volumeIndex != currentVolume) {
                writers[currentVolume]->close();
                currentVolume = entry.volumeIndex;
                currentSamplePos = 0;
                isFirstEntryInVolume = true;
            }

            // Write leading gap silence
            if (layout.gapSamples > 0) {
                writers[currentVolume]->writeSilence(layout.gapSamples);
                currentSamplePos += layout.gapSamples;
            }

            // Record begin_time in metadata
            token.entryMeta.insert("begin_time",
                                   samplesToTimecode(currentSamplePos, layout.targetFormat.kfr_format.samplerate));
            if (layout.volumes.size() > 1)
                token.entryMeta.insert("volume_index", currentVolume);

            // Write audio data
            qint64 entryFrames = 0;
            std::visit(
                [&](const auto &ptr) {
                    if (!ptr)
                        throw std::runtime_error("Null audio buffer in pipeline");
                    entryFrames = static_cast<qint64>(ptr->operator[](0).size());
                    using PtrT = std::decay_t<decltype(ptr)>;
                    if constexpr (std::is_same_v<PtrT, std::shared_ptr<utils::AudioBufferF32>>) {
                        writers[currentVolume]->writeFramesF32(*ptr);
                    } else {
                        writers[currentVolume]->writeFramesF64(*ptr);
                    }
                },
                token.processedAudio);
            currentSamplePos += entryFrames;

            // Write trailing gap silence
            if (layout.gapSamples > 0) {
                writers[currentVolume]->writeSilence(layout.gapSamples);
                currentSamplePos += layout.gapSamples;
            }

            descriptionsArray.append(token.entryMeta);
            isFirstEntryInVolume = false;
            progress.fetch_add(1, std::memory_order_relaxed);
        });

    {
        // Max live tokens controls how many audio chunks are in-flight simultaneously.
        // Higher values increase throughput but consume more memory.
        const tbb::filter<void, void> &chain = filterChain;
        tbb::parallel_pipeline(static_cast<size_t>(utils::effectiveMaxLiveTokens()), chain, ctx);
    }

    // Close remaining writers
    for (auto &w : writers) {
        if (w->isOpen())
            w->close();
    }

    // Build description JSON
    QJsonObject descObj;
    descObj.insert("version", utils::desc_file_version);
    descObj.insert("sample_rate", layout.targetFormat.kfr_format.samplerate);
    descObj.insert("sample_type", static_cast<int>(layout.targetFormat.kfr_format.type));
    descObj.insert("channel_count", static_cast<int>(layout.targetFormat.kfr_format.channels));
    descObj.insert("gap_duration", samplesToTimecode(layout.gapSamples, layout.targetFormat.kfr_format.samplerate));

    if (layout.volumes.size() == 1) {
        // Single volume — total_duration from the writer
        descObj.insert("total_duration",
                       samplesToTimecode(writers[0]->framesWritten(), layout.targetFormat.kfr_format.samplerate));
        descObj.insert("descriptions", descriptionsArray);
    } else {
        // Multi-volume
        descObj.insert("volume_count", layout.volumes.size());

        // Compute per-volume total_duration from writer frames
        QJsonArray volumesArray;
        for (size_t v = 0; v < writers.size(); ++v) {
            QJsonObject volObj;
            volObj.insert("total_duration",
                          samplesToTimecode(writers[v]->framesWritten(), layout.targetFormat.kfr_format.samplerate));
            volObj.insert("entry_begin_index", layout.volumes[v].entryIndices.first());
            volObj.insert("entry_end_index", layout.volumes[v].entryIndices.last() + 1);
            volumesArray.append(volObj);
        }
        descObj.insert("volumes", volumesArray);

        // Compute overall total_duration as sum of all volume durations
        qint64 totalFrames = 0;
        for (auto &w : writers)
            totalFrames += w->framesWritten();
        descObj.insert("total_duration", samplesToTimecode(totalFrames, layout.targetFormat.kfr_format.samplerate));

        descObj.insert("descriptions", descriptionsArray);
    }

    // Write description JSON file
    QString descFileName;
    if (layout.volumes.size() == 1) {
        descFileName = getDescFileNameFrom(layout.volumes[0].outputFilePath);
    } else {
        // For multi-volume, the desc file is named after the base filename (volume 0)
        descFileName = getDescFileNameFrom(layout.volumes[0].outputFilePath);
    }

    QFile descFile(descFileName);
    if (!descFile.open(QIODevice::WriteOnly)) {
        throw std::runtime_error(QCoreApplication::translate("WAVCombine", "Failed to write description file: %1")
                                     .arg(descFileName)
                                     .toStdString());
    }
    QJsonDocument doc(descObj);
    descFile.write(doc.toJson());
    descFile.close();

    return descObj;
}

} // namespace AudioCombine
