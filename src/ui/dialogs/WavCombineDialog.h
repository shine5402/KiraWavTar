#ifndef WAVCOMBINEDIALOG_H
#define WAVCOMBINEDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>

#include "utils/Utils.h"
#include "worker/AudioIO.h"

// TBB's profiling.h has void emit() which conflicts with Qt's 'emit' macro
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

class QLabel;
class QProgressBar;
class QDialogButtonBox;
class QTimer;

class WavCombineDialog : public QDialog
{
    Q_OBJECT
public:
    WavCombineDialog(QString rootDirName, bool recursive, const AudioIO::AudioFormat &targetFormat,
                     QString saveFileName, int gapMs = 0,
                     const utils::VolumeConfig &volumeConfig = {}, QWidget *parent = nullptr);

public slots:
    void open() override;
    int exec() override;

signals:
    void opened();

private slots:
    void startWork();
    void preCheckDone();
    void pipelineDone();

private:
    QLabel *m_label;
    QProgressBar *m_progressBar;
    QDialogButtonBox *m_buttonBox;
    QTimer *m_progressTimer = nullptr;

    QString m_rootDirName;
    bool m_recursive;
    AudioIO::AudioFormat m_targetFormat;
    QString m_saveFileName;
    int m_gapMs;
    utils::VolumeConfig m_volumeConfig;

    // Pipeline progress + cancellation
    std::shared_ptr<std::atomic<int>> m_pipelineProgress;
    std::shared_ptr<oneapi::tbb::task_group_context> m_tbbContext;
};

#endif // WAVCOMBINEDIALOG_H
