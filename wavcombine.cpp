#include "wavcombine.h"
#include <QDir>
#include <kfr/all.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "wavtar_utils.h"
#include "kfr_adapt.h"

using namespace wavtar_defines;
using namespace wavtar_utils;

//TODO: write a adapter for kfr to use QIODevice(QFile)
//FIXME: seems like it will not close files after generate..?


namespace WAVCombine {
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
