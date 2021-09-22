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

using namespace wavtar_defines;
using namespace wavtar_utils;

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

        kfr::audio_reader_wav<sample_process_t> reader(kfr::open_file_for_reading(srcWAVFileName.toStdWString()));
        auto srcData = reader.read_channels().at(0);
        //TODO:Need more check here

        //These code maybe can merged with identical codes in the combine...But with 2 occurance, whatever.
        auto descFileName = getDescFileNameFrom(srcWAVFileName);

        QFile descFileDevice{descFileName};
        if (!descFileDevice.exists())
            return;
        if (!descFileDevice.open(QFile::Text | QFile::ReadOnly))
            return;
        auto descFileRawData = descFileDevice.readAll();
        auto descJsonDoc = QJsonDocument::fromJson(descFileRawData);
        auto descJsonRoot = descJsonDoc.object();
        auto descFileVersion = descJsonRoot.value("version").toInt();

        if (descFileVersion != desc_file_version)
            return;

        auto descArray = descJsonRoot.value("description").toArray();

        for (const auto& currentDescVal : descArray){
            auto currentDesc = currentDescVal.toObject();
            auto fileName = currentDesc.value("fileName").toString();
            //may use base64 to save this for a better precision...
            auto begin_sample_index = currentDesc.value("begin_sample_index").toInt();
            auto duration = currentDesc.value("duration").toInt();

            auto currentSegmentData = kfr::univector<sample_process_t>(srcData.begin() + begin_sample_index, srcData.begin() + begin_sample_index + duration);
            //makes folder if current subdir not exist
            QFileInfo {dstDir.absoluteFilePath(fileName)}.absoluteDir().mkpath(".");

            auto writer = kfr::audio_writer_wav<sample_process_t>(kfr::open_qt_file_for_writing(dstDir.absoluteFilePath(fileName)), output_format);
            writer.write(currentSegmentData);
        }
    }

}
