#ifndef AUDIOEXTRACT_H
#define AUDIOEXTRACT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <atomic>
#include <optional>

#include "AudioIO.h"
#include "utils/KfrHelper.h"
#include "utils/Utils.h"

// TBB's profiling.h defines void emit() which conflicts with Qt's 'emit' macro.
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

namespace AudioExtract {
enum class ExtractGapMode { OriginalRange, IncludeSpace };
enum class CheckPassType { OK, WARNING, CRITICAL };

struct CheckResult
{
    CheckPassType pass;
    QString reportString;
    QJsonObject descRoot;
};

CheckResult preCheck(QString srcWAVFileName, QString dstDirName);

struct ExtractErrorDescription
{
    QString description;
    QJsonObject descObj;
};

struct ExtractPipelineParams
{
    QString srcWAVFileName;
    QJsonObject descRoot;
    QString dstDirName;
    std::optional<AudioIO::AudioFormat> targetFormat;
    bool removeDCOffset;
    ExtractGapMode gapMode;
    QString gapDurationTimecode;
    QJsonArray filteredDescArray;
};

struct ExtractPipelineResult
{
    QList<ExtractErrorDescription> errors;
};

ExtractPipelineResult runExtractPipeline(
    const ExtractPipelineParams &params,
    std::atomic<int> &progress,
    oneapi::tbb::task_group_context &ctx);

} // namespace AudioExtract

Q_DECLARE_METATYPE(AudioExtract::CheckResult);

#endif // AUDIOEXTRACT_H
