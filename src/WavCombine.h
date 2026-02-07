#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <QFuture>
#include <QObject>
#include <kfr/all.hpp>

#include "AudioIO.h"
#include "WavTarUtils.h"

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

QFuture<QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>>
startReadAndCombineWork(QStringList WAVFileNames, QString rootDirName, AudioIO::WavAudioFormat targetFormat);

bool writeCombineResult(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>> data, QJsonObject descObj,
                        QString wavFileName, AudioIO::WavAudioFormat targetFormat);
}; // namespace WAVCombine

Q_DECLARE_METATYPE(WAVCombine::CheckResult);

#endif // WAVCOMBINE_H
