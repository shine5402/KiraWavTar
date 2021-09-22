#include "wavcombine.h"
#include <QDir>
#include <kfr/all.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "wavtar_utils.h"
#include "kfr_adapt.h"
#include <QtConcurrent/QtConcurrent>
#include "lambda_wrapper.h"

using namespace wavtar_defines;
using namespace wavtar_utils;

//TODO: write a adapter for kfr to use QIODevice(QFile)
//FIXME: seems like it will not close files after generate..?
namespace WAVCombine {

    enum CheckResultType{
        OK, WARNING, CRITICAL
    };

    //@returns (canRun, checkHumanReadReportHTML)
    QPair<CheckResultType, QString> preCheck(QString rootDirName, bool recursive, kfr::audio_format targetFormat){

        auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);

        if (wavFileNames.isEmpty())
        {
            return {CRITICAL, QCoreApplication::translate("WAVCombine", "<p class='critical'>没有在所给文件夹中找到任何wav文件。请检查提供的路径，或者忘记勾选“包含子文件夹”了？</p>")};
        }

        //As mapped() need result_type member to work, so we use lambda_wrapper() to help us.
        auto formats = QtConcurrent::mapped(wavFileNames, lambda_wrapper([](const QString& fileName)->QPair<QString, kfr::audio_format_and_length>{
            kfr::audio_reader_wav<sample_process_t> reader(kfr::open_qt_file_for_reading(fileName));
            return {fileName, reader.format()};
        })).results();

        quint64 totalLength = 0;
        for (auto i : formats){
            totalLength += i.second.length;
        }
        if (totalLength > UINT32_MAX && !targetFormat.use_w64){
            return {CRITICAL, QCoreApplication::translate("WAVCombine", "<p class='critical'>合并后的音频数据长度超过了普通WAV文件所能承载的长度，请选择使用W64格式来保存。</p>")};
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
            auto isTargetIsFloatButInputNot = kfr::audio_sample_is_float(targetFormat.type) && !kfr::audio_sample_is_float(info.second.type);
            auto isTargetSizeLargger = kfr::audio_sample_sizeof(targetFormat.type) > kfr::audio_sample_sizeof(info.second.type);
            return isTargetIsFloatButInputNot || isTargetSizeLargger;
        }).results();




    }

    //TODO: Should consider error report after, ehh it's so annoying...
    void doWork(QString rootDirName, bool recursive, QString saveFileName){

        auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);

        QJsonArray combineDescArray;
        kfr::univector<sample_process_t> combinedResult;
        for (const auto& fileName : std::as_const(wavFileNames)){
            QJsonObject descObj;

            auto absoluteDirName = QDir(rootDirName).absolutePath();
            descObj.insert("fileName", fileName.mid(absoluteDirName.count() + 1));//1 for separator after dirName

            kfr::audio_reader_wav<sample_process_t> reader(kfr::open_file_for_reading(fileName.toStdWString()));

            if (reader.format().channels > 1){
                qWarning() << QString("Input file %1 has multiple channels. "
"Channels other than channel 0 will be thrown out.").arg(fileName);
            };

            if (reader.format().type != kfr::audio_sample_type::i16)
            {
                qWarning() << QString("Input file %1 are not quantized 16-bit int."
"It will be convert to 16-bit int.").arg(fileName);
            }

            // We just use one channel here.
            // We don't do the mix cuz for our input because for typical use of this tool (similar to human vocal),
            // all channels will be almost same.
            auto currentData = reader.read_channels().at(0);

            constexpr auto targetSampleRate = 44100;
            if (reader.format().samplerate != targetSampleRate){
                qWarning() << QString("Input file %1 are not sampled in 44100Hz."
"It will be convert to 44100Hz.").arg(fileName);
                auto resampler = kfr::sample_rate_converter<sample_process_t>(kfr::sample_rate_conversion_quality::perfect, targetSampleRate, reader.format().samplerate);
                decltype (currentData) resampled;
                resampler.process(resampled, currentData);
                currentData = resampled;
            }

            //may use base64 to save this for a better precision...
            descObj.insert("begin_sample_index", qint64(combinedResult.size()));
            descObj.insert("duration", qint64(currentData.size()));

            combinedResult.insert(combinedResult.end(), currentData.begin(), currentData.end());
            combineDescArray.append(descObj);
        }

        //write desc file
        auto descFileName = getDescFileNameFrom(saveFileName);
        QJsonObject descJsonRoot;
        descJsonRoot.insert("version", desc_file_version);
        descJsonRoot.insert("description", combineDescArray);
        QJsonDocument jsonDoc(descJsonRoot);
        QFile descFileDevice{descFileName};
        if (!descFileDevice.open(QFile::WriteOnly | QFile::Text))
            return;
        descFileDevice.write(jsonDoc.toJson());
        //write wave file
        kfr::audio_writer_wav<sample_process_t> writer(kfr::open_qt_file_for_writing(saveFileName), output_format);
        writer.write(combinedResult);
    }
} // namespace WAVCombineWorker
