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

namespace WAVCombine {

CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, AudioIO::WavAudioFormat targetFormat)
{

    auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);

    if (wavFileNames.isEmpty()) {
        return {CheckPassType::CRITICAL,
                QCoreApplication::translate(
                    "WAVCombine", "<p class='critical'>There are not any wav files in the given folder. "
                                  "Please check the path, or if you forget to turn \"scan subfolders\" on?</p>"),
                wavFileNames};
    }

    // As mapped() need result_type member to work, so we use std::function() to help us.
    auto formats =
        QtConcurrent::mapped(wavFileNames,
                             std::function([](const QString &fileName) -> QPair<QString, AudioIO::WavAudioFormat> {
                                 try {
                                     auto format = AudioIO::readWavFormat(fileName);
                                     return {fileName, format};
                                 } catch (...) {
                                     return {fileName, AudioIO::WavAudioFormat{}};
                                 }
                             }))
            .results();

    qint64 totalLength = 0;
    for (const auto &i : formats) {
        totalLength += i.second.length;
    }

    if (totalLength > INT32_MAX && targetFormat.container == AudioIO::WavAudioFormat::Container::RIFF) {
        return {
            CheckPassType::CRITICAL,
            QCoreApplication::translate(
                "WAVCombine",
                "<p class='critical'>Length of the wav file combined will be too large to save in normal RIFF WAVs. "
                "Please use 64-bit WAV instead.</p>"),
            wavFileNames};
    }

    QString warningMsg;

    auto hasUnknownType =
        QtConcurrent::filtered(formats, [](const QPair<QString, AudioIO::WavAudioFormat> &info) -> bool {
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
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::WavAudioFormat> &info) -> bool {
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
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::WavAudioFormat> &info) -> bool {
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
        QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, AudioIO::WavAudioFormat> &info) -> bool {
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
startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::WavAudioFormat targetFormat)
{
    const bool useDouble = utils::shouldUseDoubleInternalProcessing(targetFormat.kfr_format.type);

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
                        auto converter =
                            kfr::sample_rate_converter<T>(utils::sample_rate_conversion_quality_for_process,
                                                          (size_t)outputSampleRate, (size_t)inputSampleRate);
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

                int originalWavFormat = 0; // RIFF
                switch (readResult.format.container) {
                case AudioIO::WavAudioFormat::Container::RIFF:
                    originalWavFormat = 0;
                    break;
                case AudioIO::WavAudioFormat::Container::RF64:
                    originalWavFormat = 2;
                    break;
                case AudioIO::WavAudioFormat::Container::W64:
                    originalWavFormat = 1;
                    break;
                default:
                    break;
                }
                metaObj.insert("wav_format", originalWavFormat);
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
                auto result = AudioIO::readWavFileF64(fileName);
                return processTyped(std::move(result));
            }

            auto result = AudioIO::readWavFileF32(fileName);
            return processTyped(std::move(result));
        }),

        // Reduce
        std::function([targetFormat, useDouble](QPair<utils::AudioBufferPtr, QJsonObject> &total,
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
                jsonRoot.insert("descriptions", QJsonArray());
                total.second = jsonRoot;
            }

            size_t nChannels = targetFormat.kfr_format.channels;

            // Append Data (typed)
            size_t currentSize = 0;
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

            // Append Meta
            QJsonObject meta = input.second;
            meta.insert("begin_time", samplesToTimecode(currentSize, targetFormat.kfr_format.samplerate));

            QJsonArray arr = total.second["descriptions"].toArray();
            arr.append(meta);
            total.second.insert("descriptions", arr);

            // Update Total Duration
            total.second.insert("total_duration",
                                samplesToTimecode(currentSize + appendSize, targetFormat.kfr_format.samplerate));
        }));
}

bool writeCombineResult(utils::AudioBufferPtr data, QJsonObject descObj, QString wavFileName,
                        AudioIO::WavAudioFormat targetFormat)
{
    // Write WAV
    try {
        std::visit(
            [&](auto &ptr) {
                if (!ptr)
                    throw std::runtime_error("Null audio buffer");
                using PtrT = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<PtrT, std::shared_ptr<utils::AudioBufferF32>>) {
                    AudioIO::writeWavFileF32(wavFileName, *ptr, targetFormat);
                } else {
                    AudioIO::writeWavFileF64(wavFileName, *ptr, targetFormat);
                }
            },
            data);
    } catch (...) {
        return false;
    }

    // Write Desc
    QString descFileName = getDescFileNameFrom(wavFileName);
    QFile descFile(descFileName);
    if (!descFile.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(descObj);
    descFile.write(doc.toJson());
    descFile.close();

    return true;
}
} // namespace WAVCombine
