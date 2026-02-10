#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <QFuture>
#include <QObject>
#include <kfr/all.hpp>

#include "AudioIO.h"
#include "utils/Utils.h"

// Because we are just writing static functions, so namespace may be a better choice than class.
namespace WAVCombine {
enum class CheckPassType { OK, WARNING, CRITICAL };

struct CheckResult
{
    CheckPassType pass;
    QString reportString;
    QStringList wavFileNames;
};

CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, AudioIO::WavAudioFormat targetFormat);

QFuture<QPair<utils::AudioBufferPtr, QJsonObject>>
startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::WavAudioFormat targetFormat,
                        int gapMs = 0);

bool writeCombineResult(utils::AudioBufferPtr data, QJsonObject descObj, QString wavFileName,
                        AudioIO::WavAudioFormat targetFormat,
                        const utils::VolumeConfig &volumeConfig = {});
}; // namespace WAVCombine

Q_DECLARE_METATYPE(WAVCombine::CheckResult);

#endif // WAVCOMBINE_H
