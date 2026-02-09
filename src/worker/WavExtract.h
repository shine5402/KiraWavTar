#ifndef WAVEXTRACT_H
#define WAVEXTRACT_H

#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>

#include "AudioIO.h"
#include "utils/KfrHelper.h"
#include "utils/Utils.h"

namespace WAVExtract {
enum class ExtractGapMode { OriginalRange, IncludeSpace };
enum class CheckPassType { OK, WARNING, CRITICAL };

struct CheckResult
{
    CheckPassType pass;
    QString reportString;
    QJsonObject descRoot;
};

CheckResult preCheck(QString srcWAVFileName, QString dstDirName);

struct SrcData
{
    utils::AudioBufferPtr srcData;
    decltype(kfr::audio_format::samplerate) samplerate;
    QJsonArray descArray;
};
SrcData readSrcWAVFile(QString srcWAVFileName, QJsonObject descRoot, AudioIO::WavAudioFormat targetFormat);

struct ExtractErrorDescription
{
    QString description;
    QJsonObject descObj;
    utils::AudioBufferPtr srcData;
    decltype(kfr::audio_format::samplerate) srcSampleRate;
};
QFuture<ExtractErrorDescription> startExtract(utils::AudioBufferPtr srcData,
                                              decltype(kfr::audio_format::samplerate) srcSampleRate,
                                              QJsonArray descArray, QString dstDirName,
                                              AudioIO::WavAudioFormat targetFormat, bool removeDCOffset,
                                              ExtractGapMode gapMode = ExtractGapMode::OriginalRange,
                                              const QString &gapDurationTimecode = {});
} // namespace WAVExtract

Q_DECLARE_METATYPE(WAVExtract::CheckResult);

#endif // WAVEXTRACT_H
