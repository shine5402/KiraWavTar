#ifndef WAVEXTRACT_H
#define WAVEXTRACT_H

#include <QObject>
#include <QJsonObject>
#include "kfr_adapt.h"
#include "wavtar_utils.h"
#include <QFuture>

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
    QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonArray> readSrcWAVFile(QString srcWAVFileName, QJsonObject descRoot);
    QFuture<bool> startExtract(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> srcData, QJsonArray descArray, QString dstDirName, kfr::audio_format targetFormat);
}

Q_DECLARE_METATYPE(WAVExtract::CheckResult);

#endif // WAVEXTRACT_H
