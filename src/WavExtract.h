#ifndef WAVEXTRACT_H
#define WAVEXTRACT_H

#include <QObject>
#include <QJsonObject>
#include "WavTarUtils.h"
#include "KfrHelper.h"
#include <QFuture>
#include <QJsonArray>

namespace WAVExtract {
    enum class CheckPassType{
        OK, WARNING, CRITICAL
    };

    struct CheckResult{
        CheckPassType pass;
        QString reportString;
        QJsonObject descRoot;
    };

    CheckResult preCheck(QString srcWAVFileName, QString dstDirName);

    struct SrcData{
        std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> srcData;
        decltype (kfr::audio_format::samplerate) samplerate;
        QJsonArray descArray;
    };
    SrcData readSrcWAVFile(QString srcWAVFileName, QJsonObject descRoot);

    struct ExtractErrorDescription{
        QString description;
        QJsonObject descObj;
        std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> srcData;
        decltype (kfr::audio_format::samplerate) srcSampleRate;
    };
    QFuture<QList<ExtractErrorDescription>> startExtract(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> srcData,
                                                         decltype(kfr::audio_format::samplerate) srcSampleRate,
                                                         QJsonArray descArray, QString dstDirName, kfr::audio_format targetFormat,
                                                         bool removeDCOffset);
}

Q_DECLARE_METATYPE(WAVExtract::CheckResult);

#endif // WAVEXTRACT_H
