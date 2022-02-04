#include "wavcombine.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "wavtar_utils.h"
#include <kira/lib_helper/kfr_helper.h>
#include <kira/base64.h>
#include <kira/filesystem.h>
#include <QtConcurrent/QtConcurrent>
#include <memory>

using namespace wavtar_defines;
using namespace wavtar_utils;

namespace WAVCombine {

    CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, kfr::audio_format targetFormat){

        auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);

        if (wavFileNames.isEmpty())
        {
            return {CheckPassType::CRITICAL,
                        QCoreApplication::translate("WAVCombine",
                                                    "<p class='critical'>There are not any wav files in the given folder. "
                                                    "Please check the path, or if you forget to turn \"scan subfolders\" on?</p>"),
                        wavFileNames};
        }

        //As mapped() need result_type member to work, so we use std::function() to help us.
        auto formats = QtConcurrent::mapped(wavFileNames, std::function([](const QString& fileName)->QPair<QString, kfr::audio_format_and_length>{
            kfr::audio_reader_wav<sample_process_t> reader(kfr::open_qt_file_for_reading(fileName));
            return {fileName, reader.format()};
        })).results();

        qint64 totalLength = 0;
        for (auto i : formats){
            totalLength += i.second.length;
        }
        if (totalLength > INT32_MAX && targetFormat.wav_format == kfr::audio_format::riff){
            return {CheckPassType::CRITICAL,
                        QCoreApplication::translate("WAVCombine",
                                                    "<p class='critical'>Length of the wav file combined will be too large to save in normal RIFF WAVs. "
                                                    "Please use 64-bit WAV instead.</p>"),
                        wavFileNames};
        }

        QString warningMsg;

        auto hasUnknownType = QtConcurrent::filtered(formats, [](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.type == kfr::audio_sample_type::unknown;
        }).results();

        for (const auto& i : std::as_const(hasUnknownType)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>Can not know bit depth from \"%1\"."
                                                                        " Maybe this file id corrupted, "
                                                                        "or error happend during openning the file.</p>")
                              .arg(i.first));
        }

        auto hasMultipleChannels = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.channels > targetFormat.channels;
        }).results();

        for (const auto& i : std::as_const(hasMultipleChannels)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>There are %2 channel(s) in \"%1\". "
                                                                        "Channels after No.%3 will be discarded.</p>")
                              .arg(i.first)
                              .arg(i.second.channels)
                              .arg(targetFormat.channels));
        }

        auto hasLargerSampleRate = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.samplerate > targetFormat.samplerate;
        }).results();

        for (const auto& i : std::as_const(hasLargerSampleRate)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>Sample rate (%2 Hz) of \"%1\" is larger than target (\"%3\" Hz)."
                                                                        "The precision could be lost a bit when processing.</p>").arg(i.first).arg(i.second.samplerate).arg(targetFormat.samplerate));
        }

        auto hasLargerQuantization = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
           return (kfr::audio_sample_type_precision_length.at(info.second.type) > kfr::audio_sample_type_precision_length.at(targetFormat.type)) || (kfr::audio_sample_is_float(info.second.type) && (!kfr::audio_sample_is_float(targetFormat.type)));
        }).results();

        for (const auto& i : std::as_const(hasLargerQuantization)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>The precision could be lost a bit when processing between \"%1\" (%2) and target (%3).</p>")
                              .arg(i.first)
                              .arg(kfr::audio_sample_type_human_string.at(i.second.type).c_str())
                              .arg(kfr::audio_sample_type_human_string.at(targetFormat.type).c_str()));
        }

        auto hasLargerQuantizationThanProcess = QtConcurrent::filtered(formats, [](const QPair<QString, kfr::audio_format>& info) -> bool{
            return kfr::audio_sample_type_precision_length.at(info.second.type) > kfr::audio_sample_type_precision_length.at(sample_process_type);
         }).results();

        for (const auto& i : std::as_const(hasLargerQuantizationThanProcess)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>The precision could be lost a bit when processing, as we use a bit depth of \"%3\" in internal processing while source \"%1\" having \"%2\".</p>")
                              .arg(i.first)
                              .arg(kfr::audio_sample_type_human_string.at(i.second.type).c_str())
                              .arg(kfr::audio_sample_type_human_string.at(sample_process_type).c_str()));
        }

        if (QFileInfo{dstWAVFileName}.exists())
        {
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>The target file \"%1\" exists. "
                                                                        "It will be replaced if proceed, so BACKUP if needed to.</p>").arg(dstWAVFileName));
        }

        return {warningMsg.isEmpty() ? CheckPassType::OK : CheckPassType::WARNING, warningMsg, wavFileNames};
    }


    //(finalDataToWrite, jsonObj)
    QFuture<QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>> startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, kfr::audio_format targetFormat){
        return QtConcurrent::mappedReduced<QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>>(WAVFileNames, std::function([rootDirName, targetFormat](const QString& fileName)->QPair<kfr::univector2d<sample_process_t>, QJsonObject>{
            bool openSucess = false;
            kfr::audio_reader_wav<sample_process_t> reader(kfr::open_qt_file_for_reading(fileName, &openSucess));
            if (!openSucess)
            {
                throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "Error occurred during reading file \"%1\".").arg(fileName));
            }
            if (reader.format().type == kfr::audio_sample_type::unknown)
            {
                throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "Cannot know sample type of file \"%1\".").arg(fileName));
            }
            auto channnels = reader.read_channels();
            QJsonObject descObj;
            auto absoluteDirName = QDir(rootDirName).absolutePath();
            descObj.insert("file_name", fileName.mid(absoluteDirName.count() + 1));//1 for separator after dirName
            descObj.insert("sample_rate", reader.format().samplerate);
            descObj.insert("sample_type", (int) reader.format().type);
            descObj.insert("wav_format", reader.format().wav_format);

            //As channel count would likely not bigger enough to loose precision or overflow, so we just store it in json double. Same for sample rate.
            descObj.insert("channel_count", (qint64) (reader.format().channels < targetFormat.channels ? reader.format().channels : targetFormat.channels));
            kfr::univector2d<sample_process_t> result;
            for (decltype (targetFormat.channels) i = 0; i < targetFormat.channels; ++i){
                if (channnels.size() <= i)
                    break;
                auto srcData = channnels.at(i);
                if (reader.format().samplerate != targetFormat.samplerate){
                    //TODO: maybe give user a choice to control sample rate conversion quality
                    auto resampler = kfr::sample_rate_converter<sample_process_t>(sample_rate_conversion_quality_for_process, targetFormat.samplerate, reader.format().samplerate);
                    decltype (srcData) resampled(reader.format().length * targetFormat.samplerate / reader.format().samplerate + resampler.get_delay());
                    resampler.process(resampled, srcData);
                    srcData = resampled;
                }
                result.push_back(srcData);
            }

            //As Qt5's implementation would lose precision, we use base64 to store a int64 here.
            descObj.insert("length", encodeBase64((qint64) result.at(0).size()));

            return {result, descObj};
        }),
       std::function([targetFormat](QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>& result, const QPair<kfr::univector2d<sample_process_t>, QJsonObject>& step){
            if (!result.first)
                result.first = std::make_shared<kfr::univector2d<wavtar_defines::sample_process_t>>();
            auto descObj = step.second;
            if (descObj.isEmpty())
            {
                return;
            }
            auto& resultObj = result.second;
            auto previousCount = decodeBase64<qint64>(resultObj.value("total_length").toString());
            //We simply copy this cuz these are same...so....The first would be a empty string, though. It doesn't matter cuz we see a empty string as a 0.
            descObj.insert("begin_index", resultObj.value("total_length").toString());
            auto length = decodeBase64<qint64>(descObj.value("length").toString());
            //update the total sample count
            resultObj.insert("total_length", encodeBase64(previousCount + length));
            auto descArray = resultObj.value("descriptions").toArray();
            descArray.append(descObj);
            resultObj.insert("descriptions", descArray);

            auto& resultData = result.first;
            auto stepData = step.first;
            for (decltype (targetFormat.channels) i = 0; i < targetFormat.channels; ++i){
               //create a simple zero-data channel to meet the requirement
               if (stepData.size() <= i)
                   stepData.push_back(kfr::univector<sample_process_t>(length));
               if (resultData->size() <= i)
                   resultData->emplace_back();
               auto& channelData = (*resultData)[i];
               auto& stepChannelData = stepData[i];
               channelData.insert(channelData.end(), stepChannelData.cbegin(), stepChannelData.cend());
            }
                }),
                QtConcurrent::ReduceOption::OrderedReduce
                );
    }

    bool writeCombineResult(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> data, QJsonObject descObj, QString wavFileName, kfr::audio_format targetFormat){
        if (!data || data->empty())
        {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "No data to combine."));
        }

        //write description file
        descObj.insert("version", desc_file_version);
        descObj.insert("sample_rate", targetFormat.samplerate);
        descObj.insert("sample_type", (int) targetFormat.type);
        descObj.insert("channel_count", (qint64) targetFormat.channels);
        auto descFileName = getDescFileNameFrom(wavFileName);
        QJsonDocument descDoc{descObj};
        QFile descFileDevice{descFileName};
        if (!descFileDevice.open(QFile::WriteOnly | QFile::Text)){
            qCritical("Can not open desc json file.");
            return false;
        }
        descFileDevice.write(descDoc.toJson());
        //write wave file
        bool openSuccess = false;
        kfr::audio_writer_wav<sample_process_t> writer(kfr::open_qt_file_for_writing(wavFileName, &openSuccess), targetFormat);
        if (!openSuccess){
           throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "Error occurred during writing file \"%1\".").arg(wavFileName));
        }
        size_t to_write = 0;
        auto written = kfr::write_mutlichannel_wav_file<sample_process_t>(writer, *data, &to_write);

        if (to_write != written)
        {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "File \"%1\" can not being fully written. "
                                                                                             "%2 bytes has been written into file \"%1\", "
                                                                                             "which is expected to be %3.")
                                                   .arg(wavFileName).arg(written).arg(to_write));
        }

        return to_write == written;
    }


} // namespace WAVCombineWorker
