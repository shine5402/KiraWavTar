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

#include "AudioIO.h"
#include "utils/Filesystem.h"
#include "utils/KfrHelper.h"
#include "utils/Utils.h"

using namespace utils;

namespace AudioCombine {

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
                                                      "or error happend during openning the file.</p>")
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

    auto hasLargerQuantization =
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::AudioFormat> &info) -> bool {
            return (kfr::audio_sample_type_to_precision(info.second.kfr_format.type) >
                    kfr::audio_sample_type_to_precision(targetFormat.kfr_format.type)) ||
                   (kfr::audio_sample_is_float(info.second.kfr_format.type) &&
                    (!kfr::audio_sample_is_float(targetFormat.kfr_format.type)));
        }).results();

    for (const auto &i : std::as_const(hasLargerQuantization)) {
        warningMsg.append(QCoreApplication::translate(
                              "WAVCombine", "<p class='warning'>Bit depth (%2) of \"%1\" is larger than target."
                                            "The precision could be lost a bit when processing.</p>")
                              .arg(i.first)
                              .arg(kfr::audio_sample_type_to_string(i.second.kfr_format.type).data()));
    }

    if (warningMsg.isEmpty())
        return {CheckPassType::OK, "", wavFileNames};

    return {CheckPassType::WARNING, warningMsg, wavFileNames};
}

QFuture<QPair<utils::AudioBufferPtr, QJsonObject>>
startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::AudioFormat targetFormat, int gapMs)
{
    const bool useDouble = utils::shouldUseDoubleInternalProcessing(targetFormat.kfr_format.type);
    const qint64 gapSamples = qRound64(gapMs / 1000.0 * targetFormat.kfr_format.samplerate);

    return QtConcurrent::mappedReduced(
        WAVFileNames,
        // Map
        std::function([rootDirName, targetFormat,
                       useDouble](const QString &fileName) -> QPair<utils::AudioBufferPtr, QJsonObject> {
            // TODO: handle exceptions

            auto processTyped = [&](auto &&readResult) -> QPair<utils::AudioBufferPtr, QJsonObject> {
                using T = std::decay_t<decltype(readResult.data[0][0])>;

                auto &inputData = readResult.data;
                size_t inputChannels = readResult.format.kfr_format.channels;
                size_t inputLength = readResult.format.length;
                double inputSampleRate = readResult.format.kfr_format.samplerate;

                size_t outputChannels = targetFormat.kfr_format.channels;
                double outputSampleRate = targetFormat.kfr_format.samplerate;

                // Channel mix (Truncate extra channels, or pad with zero)
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
                        auto converter = kfr::sample_rate_converter<T>(
                            utils::getSampleRateConversionQuality(), (size_t)outputSampleRate, (size_t)inputSampleRate);
                        size_t outSize = converter.output_size_for_input(processedData[c].size());
                        resampledData[c].resize(outSize);
                        converter.process(resampledData[c], processedData[c]);
                    }
                    processedData = std::move(resampledData);
                }

                // Create Meta Object
                QJsonObject metaObj;
                metaObj.insert("file_name", QDir(rootDirName).relativeFilePath(fileName));
                metaObj.insert("sample_rate", readResult.format.kfr_format.samplerate);
                metaObj.insert("sample_type", (qint64)readResult.format.kfr_format.type);

                int originalContainerFormat = 0; // RIFF
                switch (readResult.format.container) {
                case AudioIO::AudioFormat::Container::RIFF:
                    originalContainerFormat = 0;
                    break;
                case AudioIO::AudioFormat::Container::RF64:
                    originalContainerFormat = 2;
                    break;
                case AudioIO::AudioFormat::Container::W64:
                    originalContainerFormat = 1;
                    break;
                case AudioIO::AudioFormat::Container::FLAC:
                    originalContainerFormat = 3;
                    break;
                default:
                    break;
                }
                metaObj.insert("container_format", originalContainerFormat);
                metaObj.insert("channel_count", (qint64)readResult.format.kfr_format.channels);
                metaObj.insert("duration",
                               samplesToTimecode(readResult.format.length, readResult.format.kfr_format.samplerate));

                auto retData = std::make_shared<kfr::univector2d<T>>(std::move(processedData));
                utils::AudioBufferPtr buf;
                if constexpr (std::is_same_v<T, float>)
                    buf = retData;
                else
                    buf = retData;
                return {buf, metaObj};
            };

            if (useDouble) {
                auto result = AudioIO::readAudioFileF64(fileName);
                return processTyped(std::move(result));
            }

            auto result = AudioIO::readAudioFileF32(fileName);
            return processTyped(std::move(result));
        }),

        // Reduce
        std::function([targetFormat, useDouble, gapSamples](QPair<utils::AudioBufferPtr, QJsonObject> &total,
                                                            const QPair<utils::AudioBufferPtr, QJsonObject> &input) {
            // Initialization
            if (total.second.isEmpty()) {
                if (useDouble) {
                    total.first = std::make_shared<utils::AudioBufferF64>(targetFormat.kfr_format.channels);
                } else {
                    total.first = std::make_shared<utils::AudioBufferF32>(targetFormat.kfr_format.channels);
                }

                QJsonObject jsonRoot;
                jsonRoot.insert("version", utils::desc_file_version);
                jsonRoot.insert("sample_rate", targetFormat.kfr_format.samplerate);
                jsonRoot.insert("sample_type", (int)targetFormat.kfr_format.type);
                jsonRoot.insert("channel_count", (int)targetFormat.kfr_format.channels);
                jsonRoot.insert("gap_duration", samplesToTimecode(gapSamples, targetFormat.kfr_format.samplerate));
                jsonRoot.insert("descriptions", QJsonArray());
                total.second = jsonRoot;
            }

            size_t nChannels = targetFormat.kfr_format.channels;

            // Insert leading silence padding for this entry
            size_t currentSize = 0;
            std::visit([&](auto &totalPtr, const auto &) { currentSize = totalPtr->operator[](0).size(); }, total.first,
                       input.first);

            if (gapSamples > 0) {
                std::visit(
                    [&](auto &totalPtr, const auto &) {
                        for (size_t c = 0; c < nChannels; ++c) {
                            totalPtr->operator[](c).resize(currentSize + gapSamples, 0);
                        }
                    },
                    total.first, input.first);
                currentSize += gapSamples;
            }

            // Append Data (typed)
            size_t appendSize = 0;
            std::visit(
                [&](auto &totalPtr, const auto &inputPtr) {
                    using TotalPtrT = std::decay_t<decltype(totalPtr)>;
                    using InputPtrT = std::decay_t<decltype(inputPtr)>;
                    if constexpr (std::is_same_v<TotalPtrT, InputPtrT>) {
                        currentSize = totalPtr->operator[](0).size();
                        appendSize = inputPtr->operator[](0).size();
                        for (size_t c = 0; c < nChannels; ++c) {
                            totalPtr->operator[](c).resize(currentSize + appendSize);
                            std::copy(inputPtr->operator[](c).begin(), inputPtr->operator[](c).end(),
                                      totalPtr->operator[](c).begin() + currentSize);
                        }
                    } else {
                        throw std::runtime_error("Internal buffer type mismatch");
                    }
                },
                total.first, input.first);

            // Insert trailing silence padding for this entry
            if (gapSamples > 0) {
                size_t sizeAfterAppend = 0;
                std::visit(
                    [&](auto &totalPtr, const auto &) {
                        sizeAfterAppend = totalPtr->operator[](0).size();
                        for (size_t c = 0; c < nChannels; ++c) {
                            totalPtr->operator[](c).resize(sizeAfterAppend + gapSamples, 0);
                        }
                    },
                    total.first, input.first);
            }

            // Append Meta
            QJsonObject meta = input.second;
            meta.insert("begin_time", samplesToTimecode(currentSize, targetFormat.kfr_format.samplerate));

            QJsonArray arr = total.second["descriptions"].toArray();
            arr.append(meta);
            total.second.insert("descriptions", arr);

            // Update Total Duration (includes trailing gap)
            size_t finalSize = 0;
            std::visit([&](auto &totalPtr, const auto &) { finalSize = totalPtr->operator[](0).size(); }, total.first,
                       input.first);
            total.second.insert("total_duration", samplesToTimecode(finalSize, targetFormat.kfr_format.samplerate));
        }));
}

bool writeCombineResult(utils::AudioBufferPtr data, QJsonObject descObj, QString wavFileName,
                        AudioIO::AudioFormat targetFormat, const utils::VolumeConfig &volumeConfig)
{
    double sampleRate = descObj["sample_rate"].toDouble();
    QJsonArray descriptions = descObj["descriptions"].toArray();
    QString gapDuration = descObj["gap_duration"].toString();
    qint64 gapSamples = timecodeToSamples(gapDuration, sampleRate);

    // Single-volume (no splitting)
    if (volumeConfig.mode == utils::VolumeSplitMode::None) {
        try {
            std::visit(
                [&](auto &ptr) {
                    if (!ptr)
                        throw std::runtime_error("Null audio buffer");
                    using PtrT = std::decay_t<decltype(ptr)>;
                    if constexpr (std::is_same_v<PtrT, std::shared_ptr<utils::AudioBufferF32>>) {
                        AudioIO::writeAudioFileF32(wavFileName, *ptr, targetFormat);
                    } else {
                        AudioIO::writeAudioFileF64(wavFileName, *ptr, targetFormat);
                    }
                },
                data);
        } catch (...) {
            return false;
        }

        QString descFileName = getDescFileNameFrom(wavFileName);
        QFile descFile(descFileName);
        if (!descFile.open(QIODevice::WriteOnly))
            return false;
        QJsonDocument doc(descObj);
        descFile.write(doc.toJson());
        descFile.close();
        return true;
    }

    // Multi-volume: partition entries into volumes
    struct VolumeInfo
    {
        int entryBeginIndex = 0;
        int entryEndIndex = 0;
        qint64 startSample = 0;
        qint64 endSample = 0;
    };
    QList<VolumeInfo> volumes;

    int entryCount = descriptions.size();
    if (entryCount == 0)
        return false;

    if (volumeConfig.mode == utils::VolumeSplitMode::ByCount) {
        int maxPerVol = volumeConfig.maxEntriesPerVolume;
        for (int i = 0; i < entryCount; i += maxPerVol) {
            VolumeInfo vi;
            vi.entryBeginIndex = i;
            vi.entryEndIndex = std::min(i + maxPerVol, entryCount);
            volumes.append(vi);
        }
    } else {
        // ByDuration
        qint64 maxDurationSamples =
            static_cast<qint64>(volumeConfig.maxDurationSeconds) * static_cast<qint64>(sampleRate);
        VolumeInfo currentVol;
        currentVol.entryBeginIndex = 0;
        qint64 accumulatedSamples = 0;

        for (int i = 0; i < entryCount; ++i) {
            auto entry = descriptions[i].toObject();
            qint64 entryDurSamples = timecodeToSamples(entry["duration"].toString(), sampleRate);
            qint64 footprint = 2 * gapSamples + entryDurSamples;

            if (footprint > maxDurationSamples) {
                throw std::runtime_error(
                    QCoreApplication::translate(
                        "WAVCombine",
                        "Entry \"%1\" has duration %2 which exceeds the maximum volume duration of %3 seconds.")
                        .arg(entry["file_name"].toString())
                        .arg(entry["duration"].toString())
                        .arg(volumeConfig.maxDurationSeconds)
                        .toStdString());
            }

            if (accumulatedSamples > 0 && accumulatedSamples + footprint > maxDurationSamples) {
                currentVol.entryEndIndex = i;
                volumes.append(currentVol);
                currentVol = {};
                currentVol.entryBeginIndex = i;
                accumulatedSamples = 0;
            }
            accumulatedSamples += footprint;
        }
        currentVol.entryEndIndex = entryCount;
        volumes.append(currentVol);
    }

    // Compute volume sample boundaries
    size_t totalBufferSize = 0;
    std::visit([&](const auto &ptr) { totalBufferSize = ptr->operator[](0).size(); }, data);

    for (int v = 0; v < volumes.size(); ++v) {
        auto &vol = volumes[v];
        auto firstEntry = descriptions[vol.entryBeginIndex].toObject();
        qint64 firstBeginSample = timecodeToSamples(firstEntry["begin_time"].toString(), sampleRate);
        vol.startSample = std::max(qint64(0), firstBeginSample - gapSamples);

        if (v + 1 < volumes.size()) {
            auto nextFirstEntry = descriptions[volumes[v + 1].entryBeginIndex].toObject();
            qint64 nextBeginSample = timecodeToSamples(nextFirstEntry["begin_time"].toString(), sampleRate);
            vol.endSample = std::max(qint64(0), nextBeginSample - gapSamples);
        } else {
            vol.endSample = static_cast<qint64>(totalBufferSize);
        }
    }

    // Write each volume WAV
    for (int v = 0; v < volumes.size(); ++v) {
        const auto &vol = volumes[v];
        QString volFileName = getVolumeFileName(wavFileName, v);
        qint64 sliceStart = vol.startSample;
        qint64 sliceLen = vol.endSample - vol.startSample;

        try {
            std::visit(
                [&](auto &ptr) {
                    if (!ptr)
                        throw std::runtime_error("Null audio buffer");
                    using PtrT = std::decay_t<decltype(ptr)>;
                    using T = typename PtrT::element_type::value_type::value_type;
                    size_t nChannels = ptr->size();
                    kfr::univector2d<T> slice(nChannels);
                    for (size_t c = 0; c < nChannels; ++c) {
                        slice[c] = ptr->operator[](c).slice(sliceStart, sliceLen);
                    }
                    if constexpr (std::is_same_v<T, float>) {
                        AudioIO::writeAudioFileF32(volFileName, slice, targetFormat);
                    } else {
                        AudioIO::writeAudioFileF64(volFileName, slice, targetFormat);
                    }
                },
                data);
        } catch (...) {
            return false;
        }
    }

    // Build v5 JSON
    QJsonObject v5Root;
    v5Root.insert("version", utils::desc_file_version_multivolume);
    v5Root.insert("sample_rate", descObj["sample_rate"]);
    v5Root.insert("sample_type", descObj["sample_type"]);
    v5Root.insert("channel_count", descObj["channel_count"]);
    v5Root.insert("gap_duration", descObj["gap_duration"]);
    v5Root.insert("total_duration", descObj["total_duration"]);
    v5Root.insert("volume_count", volumes.size());

    QJsonArray volumesArray;
    QJsonArray newDescriptions;

    for (int v = 0; v < volumes.size(); ++v) {
        const auto &vol = volumes[v];

        QJsonObject volObj;
        qint64 volDurationSamples = vol.endSample - vol.startSample;
        volObj.insert("total_duration", samplesToTimecode(volDurationSamples, sampleRate));
        volObj.insert("entry_begin_index", vol.entryBeginIndex);
        volObj.insert("entry_end_index", vol.entryEndIndex);
        volumesArray.append(volObj);

        for (int i = vol.entryBeginIndex; i < vol.entryEndIndex; ++i) {
            QJsonObject entry = descriptions[i].toObject();
            qint64 globalBeginSample = timecodeToSamples(entry["begin_time"].toString(), sampleRate);
            qint64 perVolumeBeginSample = globalBeginSample - vol.startSample;
            entry.insert("begin_time", samplesToTimecode(perVolumeBeginSample, sampleRate));
            entry.insert("volume_index", v);
            newDescriptions.append(entry);
        }
    }

    v5Root.insert("volumes", volumesArray);
    v5Root.insert("descriptions", newDescriptions);

    // Write single desc JSON
    QString descFileName = getDescFileNameFrom(wavFileName);
    QFile descFile(descFileName);
    if (!descFile.open(QIODevice::WriteOnly))
        return false;
    QJsonDocument doc(v5Root);
    descFile.write(doc.toJson());
    descFile.close();

    return true;
}
} // namespace AudioCombine
