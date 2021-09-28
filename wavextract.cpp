#include "wavextract.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <kfr/all.hpp>
#include <algorithm>
#include <QDir>
#include "wavtar_utils.h"
#include "kfr_adapt.h"
#include <QCoreApplication>
#include <QtConcurrent>

using namespace wavtar_defines;
using namespace wavtar_utils;

namespace WAVExtract {

    CheckResult preCheck(QString srcWAVFileName, QString dstDirName)
    {
        auto descFileName = getDescFileNameFrom(srcWAVFileName);

        if (!(QFileInfo{srcWAVFileName}.exists() && QFileInfo{descFileName}.exists()))
            return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVExtract", "<p class='critical'>提供的波形文件或者其对应的描述文件不存在。</p>"), {}};

        QFile descFileDevice{descFileName};

        if (!descFileDevice.open(QFile::Text | QFile::ReadOnly))
            return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVExtract", "<p class='critical'>无法打开描述文件。</p>"), {}};

        auto descFileByteData = descFileDevice.readAll();
        auto descJsonDoc = QJsonDocument::fromJson(descFileByteData);
        auto descRoot = descJsonDoc.object();

        if (descRoot.value("version").toInt() != desc_file_version)
             return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVExtract", "<p class='critical'>描述文件的版本和我们当前使用的不兼容。</p>"), {}};

        bool openSuccess = false;
        kfr::audio_reader_wav<sample_process_t> reader(kfr::open_qt_file_for_reading(srcWAVFileName, &openSuccess));
        if (!openSuccess)
            return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVExtract", "<p class='critical'>无法打开波形文件。</p>"), {}};

        QString warningMsg;
        auto totalLength = decodeBase64<qint64>(descRoot.value("total_length").toString());
        if (reader.format().length != totalLength)
            warningMsg.append(QCoreApplication::translate("WAVExtract", "<p class='warning'>波形文件的长度“%1”和描述文件告知的“%2”不一致。修改了采样率还是进行了时值修改？</p>").arg(reader.format().length).arg(totalLength));
        auto expectedSampleRate = descRoot.value("sample_rate").toDouble();
        if (reader.format().samplerate != expectedSampleRate)
            warningMsg.append(QCoreApplication::translate("WAVExtract", "<p class='warning'>波形文件的采样率“%1 Hz”和描述文件告知“%2 Hz”的不一致。我们会进行重采样。</p>").arg(reader.format().samplerate).arg(expectedSampleRate));
        auto expectedChannelCount = descRoot.value("channel_count").toInt();
        if ((qint64) reader.format().channels != expectedChannelCount)
            warningMsg.append(QCoreApplication::translate("WAVExtract", "<p class='warning'>波形文件的声道数“%1”与预期的“%2”不符。</p>").arg(reader.format().channels).arg(expectedChannelCount));

        if (!QDir{dstDirName}.isEmpty())
            warningMsg.append(QCoreApplication::translate("WAVExtract", "<p class='warning'>看起来目标文件夹不为空。在拆分过程中所有会产生名称冲突的文件都会被替换，如果有必要的话请做好备份。</p>"));

        return {warningMsg.isEmpty() ? CheckPassType::OK : CheckPassType::WARNING, warningMsg, descRoot};
    }

    QPair<std::shared_ptr<kfr::univector2d<sample_process_t>>, QJsonArray> readSrcWAVFile(QString srcWAVFileName, QJsonObject descRoot){
        bool openSuccess = false;
        kfr::audio_reader_wav<sample_process_t> reader{kfr::open_qt_file_for_reading(srcWAVFileName, &openSuccess)};
        if (!openSuccess){
            qCritical("Can not open wave file.");
            return {};
        }
        auto data = std::make_shared<kfr::univector2d<sample_process_t>>(reader.read_channels());
        if (data->empty()){
            qCritical("There is no data.");
            return {};
        }

        auto expectedChannelCount = descRoot.value("channel_count").toInt();
        if ((qint64) reader.format().channels < expectedChannelCount)
        {
            for (auto i = 0; i < expectedChannelCount - (qint64) reader.format().channels; ++i){
                       data->emplace_back(reader.format().length);
                   }
        }
        else if ((qint64) reader.format().channels > expectedChannelCount){
            data->erase(data->begin() + expectedChannelCount, data->end());
        }


        auto expectedSampleRate = descRoot.value("sample_rate").toDouble();
        if (reader.format().samplerate != expectedSampleRate)
        {
            for (auto it = data->begin(); it != data->end(); ++it){
                auto& srcData = *it;
                auto resampler = kfr::sample_rate_converter<sample_process_t>(sample_rate_conversion_quality_for_process, expectedSampleRate, reader.format().samplerate);
                kfr::univector<sample_process_t> resampled(reader.format().length * expectedSampleRate / reader.format().samplerate + resampler.get_delay());
                resampler.process(resampled, srcData);
                //We remove delay here for a precise cut based on description file
                srcData = kfr::univector<sample_process_t>(resampled.begin() + resampler.get_delay(), resampled.end());
            }
        }

        auto expectedLength = decodeBase64<qint64>(descRoot.value("total_length").toString());
        if (reader.format().length != expectedLength){
            for (auto it = data->begin(); it != data->end(); ++it){
                kfr::univector<sample_process_t> trimmed(expectedLength);
                auto& src = *it;
                auto srcEnd = ((qint64) src.size() < expectedLength) ? src.end() : src.begin() + expectedLength;
                std::copy(src.begin(), srcEnd, trimmed.begin());
                src = trimmed;
            }
        }

        return {data, descRoot.value("descriptions").toArray()};
    }

    //Use format in description file, when targetFormat's type == unknown
    QFuture<bool> startExtract(std::shared_ptr<kfr::univector2d<sample_process_t>> srcData, QJsonArray descArray, QString dstDirName, kfr::audio_format targetFormat){
        //TODO:Use exception for error handling..?

        auto descArrayCount = descArray.count();
        //We change the array to VariantList here, as Qt 5.15.2 seems not copy QJsonArray correctly here. It may be a bug with the implicit sharing with the QJsonArray.
        return QtConcurrent::mappedReduced<bool>(descArray.toVariantList(), std::function([targetFormat, srcData, dstDirName](const QVariant& descValue) mutable -> bool{
            auto descObj = descValue.toJsonObject();

            //Use format from desc file if user ask to
            if (targetFormat.type == kfr::audio_sample_type::unknown){
                targetFormat.samplerate = descObj.value("sample_rate").toDouble();
                targetFormat.type = (kfr::audio_sample_type) descObj.value("sample_type").toInt();
                targetFormat.use_w64 = descObj.value("use_w64").toBool();
                targetFormat.channels = descObj.value("channel_count").toInt();
            }

            auto beginIndex = decodeBase64<qint64>(descObj.value("begin_index").toString());
            auto length = decodeBase64<qint64>(descObj.value("length").toString());
            auto segmentData = kfr::univector2d<sample_process_t>(targetFormat.channels);
            for (decltype (targetFormat.channels) i = 0; i < targetFormat.channels; ++i){
                auto& segmentChannel = segmentData[i];
                segmentChannel = kfr::univector<sample_process_t>(srcData->at(i).begin() + beginIndex, srcData->at(i).begin() + beginIndex + length);
            }

            auto fileName = descObj.value("file_name").toString();
            auto absoluteFileName = QDir(dstDirName).absoluteFilePath(fileName);
            //makes folder if current subdir not exist
            QFileInfo {absoluteFileName}.absoluteDir().mkpath(".");
            bool openSuccess;
            auto writer = kfr::audio_writer_wav<sample_process_t>(kfr::open_qt_file_for_writing(absoluteFileName, &openSuccess), targetFormat);
            if (!openSuccess){
                qCritical("Can not open file to write.");
                return false;
            }

            return kfr::write_mutlichannel_wav_file<sample_process_t>(writer, segmentData);
        }),
        std::function([descArrayCount](bool& result, const bool& stepResult){
            static auto count = 0;
            if (stepResult) ++count;
            if (descArrayCount == count) result = true;
        }));
    }
}
