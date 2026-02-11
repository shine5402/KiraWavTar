#ifndef WAVEXTRACTDIALOG_H
#define WAVEXTRACTDIALOG_H

#include <QDialog>
#include <atomic>
#include <kfr/all.hpp>
#include <memory>
#include <optional>

#include "utils/Utils.h"
#include "worker/AudioIO.h"
#include "worker/AudioExtract.h"

// TBB's profiling.h has void emit() which conflicts with Qt's 'emit' macro
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

class QLabel;
class QProgressBar;
class QDialogButtonBox;
class QTimer;

class WavExtractDialog : public QDialog
{
    Q_OBJECT

public:
    WavExtractDialog(QString srcWAVFileName, QString dstDirName,
                     const std::optional<AudioIO::AudioFormat> &targetFormat, bool extractResultSelection,
                     bool removeDCOffset, QWidget *parent);

public slots:
    void open() override;
    int exec() override;

signals:
    void opened();

private:
    void launchPipeline(QJsonArray filteredDescArray);
    void handlePipelineResult(const AudioExtract::ExtractPipelineResult &result);

private slots:
    void startWork();
    void preCheckDone();
    void pipelineDone();

private:
    QLabel *m_label;
    QProgressBar *m_progressBar;
    QDialogButtonBox *m_buttonBox;
    QTimer *m_progressTimer = nullptr;

    QString m_srcWAVFileName;
    QString m_dstDirName;
    std::optional<AudioIO::AudioFormat> m_targetFormat;
    bool m_extractResultSelection;
    bool m_removeDCOffset;
    AudioExtract::ExtractGapMode m_extractGapMode = AudioExtract::ExtractGapMode::OriginalRange;
    QString m_gapDurationTimecode;
    QStringList m_allVolumeFiles;
    QJsonObject m_descRoot;

    // Pipeline progress + cancellation
    std::shared_ptr<std::atomic<int>> m_pipelineProgress;
    std::shared_ptr<oneapi::tbb::task_group_context> m_tbbContext;
};

#endif // WAVEXTRACTDIALOG_H
