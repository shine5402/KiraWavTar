#include "WavCombineDialog.h"
#include "CommonHtmlDialog.h"

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

#include "kfr/all.hpp"
#include "worker/WavCombine.h"

using namespace AudioCombine;
using namespace utils;

WavCombineDialog::WavCombineDialog(QString rootDirName, bool recursive, const AudioIO::AudioFormat &targetFormat,
                                   QString saveFileName, int gapMs,
                                   const utils::VolumeConfig &volumeConfig, QWidget *parent)
    : QDialog(parent), m_rootDirName(rootDirName), m_recursive(recursive), m_targetFormat(targetFormat),
      m_saveFileName(saveFileName), m_gapMs(gapMs), m_volumeConfig(volumeConfig)
{
    auto layout = new QVBoxLayout(this);

    m_label = new QLabel(this);
    layout->addWidget(m_label);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_progressBar);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    layout->addWidget(m_buttonBox);

    setLayout(layout);
    resize(300, height());

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &WavCombineDialog::opened, this, &WavCombineDialog::startWork);
}

using PreCheckFutureWatcher = QFutureWatcher<decltype(std::function(preCheck))::result_type>;

void WavCombineDialog::startWork()
{
    auto nextFuture = QtConcurrent::run(preCheck, m_rootDirName, m_saveFileName, m_recursive, m_targetFormat);
    m_label->setText(tr("Some preparing work..."));
    // This will make progress bar show as busy indicator
    m_progressBar->setMaximum(0);
    m_progressBar->setMinimum(0);
    auto watcher = new PreCheckFutureWatcher(this);
    watcher->setFuture(nextFuture);
    connect(watcher, &PreCheckFutureWatcher::finished, this, &WavCombineDialog::preCheckDone);
    connect(m_buttonBox, &QDialogButtonBox::rejected, watcher, &PreCheckFutureWatcher::cancel);
}

void WavCombineDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        switch (result.pass) {
        case AudioCombine::CheckPassType::CRITICAL: {
            CommonHtmlDialog dlg(this);
            dlg.setWindowTitle(tr("Error"));
            dlg.setLabel(tr("Critical error found. Can not continue."));
            dlg.setHTML(reportTextStyle + result.reportString);
            dlg.setStandardButtons(QDialogButtonBox::Ok);
            dlg.exec();
            done(QDialog::Rejected);
            break;
        }
        case AudioCombine::CheckPassType::WARNING: {
            CommonHtmlDialog dlg(this);
            dlg.setWindowTitle(tr("Warning"));
            dlg.setLabel(tr("Some problems have been found but process can continue. Should we proceed?"));
            dlg.setHTML(reportTextStyle + result.reportString);
            dlg.setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            if (dlg.exec() != QDialog::Accepted) {
                this->done(QDialog::Rejected);
                this->close();
                return;
            }
        }
            [[fallthrough]];
        case AudioCombine::CheckPassType::OK: {
            // Compute layout (may throw for ByDuration violations)
            CombineLayout combineLayout;
            try {
                combineLayout = computeLayout(result.wavFileNames, m_rootDirName, m_saveFileName,
                                              m_targetFormat, m_gapMs, m_volumeConfig);
            } catch (const std::exception &e) {
                QMessageBox::critical(this, tr("Error"), QString::fromStdString(e.what()));
                done(QDialog::Rejected);
                return;
            }

            // Set up progress tracking
            m_pipelineProgress = std::make_shared<std::atomic<int>>(0);
            m_tbbContext = std::make_shared<oneapi::tbb::task_group_context>();

            int totalEntries = combineLayout.entries.size();
            m_label->setText(tr("Reading, processing and writing audio files..."));
            m_progressBar->setMinimum(0);
            m_progressBar->setMaximum(totalEntries);
            m_progressBar->setValue(0);

            // Start a timer to poll atomic progress
            m_progressTimer = new QTimer(this);
            auto progressPtr = m_pipelineProgress;
            connect(m_progressTimer, &QTimer::timeout, this, [this, progressPtr]() {
                m_progressBar->setValue(progressPtr->load(std::memory_order_relaxed));
            });
            m_progressTimer->start(100);

            // Connect cancel button to TBB cancellation
            auto tbbCtx = m_tbbContext;
            connect(m_buttonBox, &QDialogButtonBox::rejected, this, [tbbCtx]() {
                tbbCtx->cancel_group_execution();
            });

            // Run pipeline on worker thread
            auto nextFuture = QtConcurrent::run([combineLayout, progressPtr, tbbCtx]() {
                return runCombinePipeline(combineLayout, *progressPtr, *tbbCtx);
            });
            auto nextWatcher = new QFutureWatcher<QJsonObject>(this);
            nextWatcher->setFuture(nextFuture);
            connect(nextWatcher, &QFutureWatcher<QJsonObject>::finished, this,
                    &WavCombineDialog::pipelineDone);
            break;
        }
        }
    }
}

void WavCombineDialog::pipelineDone()
{
    // Stop progress polling
    if (m_progressTimer) {
        m_progressTimer->stop();
        m_progressTimer->deleteLater();
        m_progressTimer = nullptr;
    }

    if (auto watcher = dynamic_cast<QFutureWatcher<QJsonObject> *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future())) {
            done(QDialog::Rejected);
            return;
        }
        if (watcher->isCanceled() ||
            (m_tbbContext && m_tbbContext->is_group_execution_cancelled())) {
            done(QDialog::Rejected);
            return;
        }

        m_label->setText(tr("Done"));
        m_progressBar->setValue(m_progressBar->maximum());

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Icon::Information);
        msgBox.setText(tr("Wav files has been combined."));
        msgBox.setInformativeText(
            tr("Combined file has been stored at \"%1\"."
               "Please do not change the time when you edit it, "
               "and do not delete or modify \".kirawavtar-desc.json\" file sharing the same name with the WAV.")
                .arg(m_saveFileName));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        done(QDialog::Accepted);
    }
}

void WavCombineDialog::open()
{
    emit opened();
    QDialog::open();
}

int WavCombineDialog::exec()
{
    emit opened();
    return QDialog::exec();
}
