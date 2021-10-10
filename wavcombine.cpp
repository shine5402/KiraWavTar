#include "wavcombine.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "wavtar_utils.h"
#include "kfr_adapt.h"
#include <QtConcurrent/QtConcurrent>
#include <memory>

using namespace wavtar_defines;
using namespace wavtar_utils;

namespace WAVCombine {

    CheckResult preCheck(QString rootDirName, bool recursive, kfr::audio_format targetFormat){

        auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);

        if (wavFileNames.isEmpty())
        {
            return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVCombine", "<p class='critical'>没有在所给文件夹中找到任何wav文件。请检查提供的路径，或者忘记勾选“包含子文件夹”了？</p>"), wavFileNames};
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
        if (totalLength > INT32_MAX && !targetFormat.use_w64){
            return {CheckPassType::CRITICAL, QCoreApplication::translate("WAVCombine", "<p class='critical'>合并后的音频数据长度超过了普通WAV文件所能承载的长度，请选择使用W64格式来保存。</p>"), wavFileNames};
        }

        QString warningMsg;

        auto hasUnknownType = QtConcurrent::filtered(formats, [](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.type == kfr::audio_sample_type::unknown;
        }).results();

        for (const auto& i : std::as_const(hasUnknownType)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>我们无法得知“%1”存储的量化类型，可能该文件已损坏，或者文件打开时出现了问题。</p>").arg(i.first));
        }

        auto hasMultipleChannels = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.channels > targetFormat.channels;
        }).results();

        for (const auto& i : std::as_const(hasMultipleChannels)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>“%1”包含了%2个声道，位于%3号之后的声道数据会被丢弃。</p>").arg(i.first).arg(i.second.channels).arg(targetFormat.channels));
        }

        auto hasLargerSampleRate = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
            return info.second.samplerate > targetFormat.samplerate;
        }).results();

        for (const auto& i : std::as_const(hasLargerSampleRate)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>“%1”的采样率（%2 Hz）比目标（%3 Hz）要大，处理时的重采样会造成一定的精度损失。</p>").arg(i.first).arg(i.second.samplerate).arg(targetFormat.samplerate));
        }

        auto hasLargerQuantization = QtConcurrent::filtered(formats, [targetFormat](const QPair<QString, kfr::audio_format>& info) -> bool{
           return kfr::audio_sample_type_precision_length.at(info.second.type) > kfr::audio_sample_type_precision_length.at(targetFormat.type);//FIXME:make think of float -> int
        }).results();

        for (const auto& i : std::as_const(hasLargerQuantization)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>“%1”的量化类型“%2”在转换到目标“%3”时可能会损失精度，或发生溢出。</p>")
                              .arg(i.first)
                              .arg(kfr::audio_sample_type_human_string.at(i.second.type).c_str())
                              .arg(kfr::audio_sample_type_human_string.at(targetFormat.type).c_str()));
        }

        auto hasLargerQuantizationThanProcess = QtConcurrent::filtered(formats, [](const QPair<QString, kfr::audio_format>& info) -> bool{
            return kfr::audio_sample_type_precision_length.at(info.second.type) > kfr::audio_sample_type_precision_length.at(sample_process_type);
         }).results();

        for (const auto& i : std::as_const(hasLargerQuantization)){
            warningMsg.append(QCoreApplication::translate("WAVCombine", "<p class='warning'>“%1”的量化类型“%2”在处理时可能会损失精度，因为本程序内部统一使用”%3“来进行处理。</p>")
                              .arg(i.first)
                              .arg(kfr::audio_sample_type_human_string.at(i.second.type).c_str())
                              .arg(kfr::audio_sample_type_human_string.at(sample_process_type).c_str()));
        }

        return {warningMsg.isEmpty() ? CheckPassType::OK : CheckPassType::WARNING, warningMsg, wavFileNames};
    }


    //(finalDataToWrite, jsonObj)
    QFuture<QPair<kfr::univector2d<wavtar_defines::sample_process_t>, QJsonObject>> startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, kfr::audio_format targetFormat){
        return QtConcurrent::mappedReduced<QPair<kfr::univector2d<sample_process_t>, QJsonObject>>(WAVFileNames, std::function([rootDirName, targetFormat](const QString& fileName)->QPair<kfr::univector2d<sample_process_t>, QJsonObject>{
            bool openSucess = false;
            kfr::audio_reader_wav<sample_process_t> reader(kfr::open_qt_file_for_reading(fileName, &openSucess));
            if (!openSucess)
            {
                throw wavtar_exceptions::runtime_error(QCoreApplication::tr("WAVCombine", "为读取打开文件%1时出现错误。").arg(fileName));
            }
            if (reader.format().type == kfr::audio_sample_type::unknown)
            {
                throw wavtar_exceptions::runtime_error(QCoreApplication::tr("WAVCombine", "无法得知文件%1的采样类型。").arg(fileName));
            }
            auto channnels = reader.read_channels();
            QJsonObject descObj;
            auto absoluteDirName = QDir(rootDirName).absolutePath();
            descObj.insert("file_name", fileName.mid(absoluteDirName.count() + 1));//1 for separator after dirName
            descObj.insert("sample_rate", reader.format().samplerate);
            descObj.insert("sample_type", (int) reader.format().type);
            descObj.insert("use_w64", reader.format().use_w64);

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
       std::function([targetFormat](QPair<kfr::univector2d<sample_process_t>, QJsonObject>& result, const QPair<kfr::univector2d<sample_process_t>, QJsonObject>& step){
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
               if (resultData.size() <= i)
                   resultData.emplace_back();
               auto& channelData = resultData[i];
               auto& stepChannelData = stepData[i];
               channelData.insert(channelData.end(), stepChannelData.cbegin(), stepChannelData.cend());
            }
                }),
                QtConcurrent::ReduceOption::OrderedReduce
                );
    }

    bool writeCombineResult(kfr::univector2d<sample_process_t> data, QJsonObject descObj, QString wavFileName, kfr::audio_format targetFormat){
        if (data.empty())
        {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "没有可被合并的数据。"));
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
           throw wavtar_exceptions::runtime_error(QCoreApplication::tr("WAVCombine", "为写入打开文件%1时出现错误。").arg(wavFileName));
        }
        size_t to_write = 0;
        auto written = kfr::write_mutlichannel_wav_file<sample_process_t>(writer, data, &to_write);

        if (to_write != written)
        {
            throw wavtar_exceptions::runtime_error(QCoreApplication::translate("WAVCombine", "文件%1写入的字节数（%2）和预期的（%3）不一致（即没有完全写入完成）。").arg(wavFileName).arg(written).arg(to_write));
        }

        return to_write == written;
    }


} // namespace WAVCombineWorker
