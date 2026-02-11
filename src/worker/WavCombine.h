#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <atomic>
#include <kfr/all.hpp>

#include "AudioIO.h"
#include "utils/Utils.h"

// TBB's profiling.h defines void emit() which conflicts with Qt's 'emit' macro.
// Temporarily undefine it for the TBB include.
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

// Because we are just writing static functions, so namespace may be a better choice than class.
namespace AudioCombine {
enum class CheckPassType { OK, WARNING, CRITICAL };

struct CheckResult
{
    CheckPassType pass;
    QString reportString;
    QStringList wavFileNames;
};

CheckResult preCheck(QString rootDirName, QString dstWAVFileName, bool recursive, AudioIO::AudioFormat targetFormat);

// --- Layout types for streaming pipeline ---

struct EntryLayout {
    QString filePath;            // Absolute path to source file
    QString relativePath;        // Relative to rootDir (for JSON)
    AudioIO::AudioFormat sourceFormat;
    int volumeIndex = 0;         // Which volume this entry belongs to
};

struct VolumeLayout {
    QString outputFilePath;      // e.g. "output_0.wav" or just "output.wav" for single volume
    QList<int> entryIndices;     // Indices into entries list
    qint64 totalFrames = 0;      // Total output frames (entries + gaps) — hint for writer
};

struct CombineLayout {
    QList<EntryLayout> entries;
    QList<VolumeLayout> volumes; // Size 1 for single-volume
    AudioIO::AudioFormat targetFormat;
    qint64 gapSamples = 0;
    bool useDouble = false;      // F64 internal processing?
    QString rootDirName;
};

// Compute the full output layout from preCheck results.
CombineLayout computeLayout(const QStringList &wavFileNames, const QString &rootDirName,
                             const QString &saveFileName,
                             const AudioIO::AudioFormat &targetFormat,
                             int gapMs,
                             const utils::VolumeConfig &volumeConfig);

// Runs the streaming combine pipeline: read → process → write.
// Blocks until complete (call from a worker thread).
// progress: atomic counter incremented after each entry is written.
// ctx: TBB task group context for cancellation from another thread.
// Returns the description JSON object on success, throws on failure.
QJsonObject runCombinePipeline(
    const CombineLayout &layout,
    std::atomic<int> &progress,
    oneapi::tbb::task_group_context &ctx);

}; // namespace AudioCombine

Q_DECLARE_METATYPE(AudioCombine::CheckResult);

#endif // WAVCOMBINE_H
