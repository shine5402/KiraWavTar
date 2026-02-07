#include "WavCombine.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrent>
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <memory>

#include "AudioIO.h"
#include "Filesystem.h"
#include "KfrHelper.h"
#include "Utils.h"

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

QFuture<QPair<std::shared_ptr<kfr::univector2d<utils::sample_process_t>>, QJsonObject>>
startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::WavAudioFormat targetFormat)
{
    return QtConcurrent::mappedReduced(
        WAVFileNames,
        // Map
        std::function([rootDirName, targetFormat](const QString &fileName)
                          -> QPair<std::shared_ptr<kfr::univector2d<utils::sample_process_t>>, QJsonObject> {
            // TODO: handle exceptions

            auto result = AudioIO::readWavFile(fileName);
            // KFR has no built-in resampling in `read_file` anymore since we use `dr_wav` manually.
            // But we must support resampling and channel mixing.

            // We read as float32/double (sample_process_t -> float).
            // Input data is in result.data (univector2d<float>).

            auto &inputData = result.data;
            size_t inputChannels = result.format.kfr_format.channels;
            size_t inputLength = result.format.length;
            double inputSampleRate = result.format.kfr_format.samplerate;

            size_t outputChannels = targetFormat.kfr_format.channels;
            double outputSampleRate = targetFormat.kfr_format.samplerate;

            // Channel mix (Truncate extra channels, or pad with zero)
            kfr::univector2d<utils::sample_process_t> processedData(outputChannels);
            for (size_t c = 0; c < outputChannels; ++c) {
                if (c < inputChannels) {
                    processedData[c] = inputData[c]; // Copy existing channel
                } else {
                    processedData[c].resize(inputLength, 0.0f); // Zero pad
                }
            }

            // Resample if needed
            if (std::abs(inputSampleRate - outputSampleRate) > 1e-5) {
                using T = utils::sample_process_t;

                kfr::univector2d<T> resampledData(outputChannels);
                for (size_t c = 0; c < outputChannels; ++c) {
                    if (c >= processedData.size())
                        continue;

                    auto converter = kfr::sample_rate_converter<T>(kfr::sample_rate_conversion_quality::normal,
                                                                   (size_t)outputSampleRate, (size_t)inputSampleRate);
                    size_t outSize = converter.output_size_for_input(processedData[c].size());

                    resampledData[c].resize(outSize);
                    converter.process(resampledData[c], processedData[c]);
                }
                processedData = resampledData;
            }

            // Create Meta Object
            QJsonObject metaObj;
            metaObj.insert("file_name", QDir(rootDirName).relativeFilePath(fileName));
            metaObj.insert("sample_rate", result.format.kfr_format.samplerate);
            metaObj.insert("sample_type", (qint64)result.format.kfr_format.type);

            int originalWavFormat = 0; // RIFF
            switch (result.format.container) {
            case AudioIO::WavAudioFormat::Container::RIFF:
                originalWavFormat = 0;
                break; // kfr::audio_format::riff
            case AudioIO::WavAudioFormat::Container::RF64:
                originalWavFormat = 2;
                break; // kfr::audio_format::rf64
            case AudioIO::WavAudioFormat::Container::W64:
                originalWavFormat = 1;
                break; // kfr::audio_format::w64
            default:
                break;
            }
            metaObj.insert("wav_format", originalWavFormat);
            metaObj.insert("channel_count", (qint64)result.format.kfr_format.channels);
            metaObj.insert("duration", samplesToTimecode(result.format.length, result.format.kfr_format.samplerate));

            // Return data and meta
            // Note: `processedData` is the data we want to keep.
            // We need to move it to shared_ptr.
            auto retData = std::make_shared<kfr::univector2d<utils::sample_process_t>>(std::move(processedData));

            return {retData, metaObj};
        }),

        // Reduce
        std::function([targetFormat](
                          QPair<std::shared_ptr<kfr::univector2d<utils::sample_process_t>>, QJsonObject> &total,
                          const QPair<std::shared_ptr<kfr::univector2d<utils::sample_process_t>>, QJsonObject> &input) {
            // Initialization
            if (total.first == nullptr) {
                total.first =
                    std::make_shared<kfr::univector2d<utils::sample_process_t>>(targetFormat.kfr_format.channels);

                QJsonObject jsonRoot;
                jsonRoot.insert("version", utils::desc_file_version);
                jsonRoot.insert("sample_rate", targetFormat.kfr_format.samplerate);
                jsonRoot.insert("sample_type", (int)targetFormat.kfr_format.type);
                jsonRoot.insert("channel_count", (int)targetFormat.kfr_format.channels);
                jsonRoot.insert("descriptions", QJsonArray());
                total.second = jsonRoot;
            }

            // Append Data
            size_t nChannels = targetFormat.kfr_format.channels;
            size_t currentSize = total.first->operator[](0).size();
            size_t appendSize = input.first->operator[](0).size();

            for (size_t c = 0; c < nChannels; ++c) {
                // Resize and copy
                total.first->operator[](c).resize(currentSize + appendSize);
                std::copy(input.first->operator[](c).begin(), input.first->operator[](c).end(),
                          total.first->operator[](c).begin() + currentSize);
            }

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

bool writeCombineResult(std::shared_ptr<kfr::univector2d<utils::sample_process_t>> data, QJsonObject descObj,
                        QString wavFileName, AudioIO::WavAudioFormat targetFormat)
{
    if (!data)
        return false;

    // Write WAV
    try {
        AudioIO::writeWavFile(wavFileName, *data, targetFormat);
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
