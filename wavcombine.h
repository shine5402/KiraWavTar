#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <kfr/all.hpp>
#include <QObject>

#include "wavtar_utils.h"
#include <QFuture>

//Because we are just writing static functions, so namespace may be a better choice than class.
namespace WAVCombine
{
    enum class CheckPassType{
        OK, WARNING, CRITICAL
    };

    struct CheckResult{
        CheckPassType pass;
        QString reportString;
        QStringList wavFileNames;
    };

    CheckResult preCheck(QString rootDirName, bool recursive, kfr::audio_format targetFormat);
    QFuture<QPair<kfr::univector2d<wavtar_defines::sample_process_t>, QJsonObject>> startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, kfr::audio_format targetFormat);
    bool writeCombineResult(kfr::univector2d<wavtar_defines::sample_process_t> data, QJsonObject descObj, QString wavFileName, kfr::audio_format targetFormat);
};

Q_DECLARE_METATYPE(WAVCombine::CheckResult);

#endif // WAVCOMBINE_H
