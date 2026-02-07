#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <kfr/all.hpp>
#include <QObject>
#include "AudioIO.h"

#include "WavTarUtils.h"
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

    CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, AudioIO::WavAudioFormat targetFormat);

    QFuture<QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>> startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::WavAudioFormat targetFormat);

    bool writeCombineResult(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> data, QJsonObject descObj, QString wavFileName, AudioIO::WavAudioFormat targetFormat);
};

Q_DECLARE_METATYPE(WAVCombine::CheckResult);

#endif // WAVCOMBINE_H
