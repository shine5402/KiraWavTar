#include "wavextract.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <kfr/all.hpp>
#include <algorithm>
#include <QDir>

namespace WAVExtact {

    //TODO: should consider error report after...
    //TODO: ...and we better have make it multithreaded, at least we should process events to prevent freezing...
    void doWork(QString srcWAVFileName, QString dstDirName)
    {
        QDir dstDir{dstDirName};
        if (!dstDir.exists())
        {
            dstDir.mkpath(".");
        }

        kfr::audio_reader_wav<float> reader(kfr::open_file_for_reading(srcWAVFileName.toStdWString()));
        auto srcData = reader.read_channels().at(0);
        //TODO:Need more check here

        //These code maybe can merged with identical codes in the combine...But with 2 occurance, whatever.
        auto descFileName = srcWAVFileName;
        descFileName.remove(QRegExp("(\\.wav)$"));
        descFileName.append(".kirawavtar-desc.json");

        QFile descFileDevice{descFileName};
        if (!descFileDevice.exists())
            return;
        if (!descFileDevice.open(QFile::Text | QFile::ReadOnly))
            return;
        auto descFileRawData = descFileDevice.readAll();
        auto descJsonDoc = QJsonDocument::fromJson(descFileRawData);
        auto descArray = descJsonDoc.array();

        for (const auto& currentDescVal : descArray){
            auto currentDesc = currentDescVal.toObject();
            auto fileName = currentDesc.value("fileName").toString();
            //may use base64 to save this for a better precision...
            auto begin_sample_index = currentDesc.value("begin_sample_index").toInt();
            auto duration = currentDesc.value("duration").toInt();

            auto currentSegmentData = kfr::univector<float>(srcData.begin() + begin_sample_index, srcData.begin() + begin_sample_index + duration);
            //makes folder if current subdir not exist
            QFileInfo {dstDir.absoluteFilePath(fileName)}.absoluteDir().mkpath(".");

            auto writer = kfr::audio_writer_wav<float>(kfr::open_file_for_writing(dstDir.absoluteFilePath(fileName).toStdWString()), {1, kfr::audio_sample_type::i16, 44100, false});
            writer.write(currentSegmentData);
        }
    }

}
