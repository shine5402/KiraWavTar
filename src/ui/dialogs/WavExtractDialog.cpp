#include "WavExtractDialog.h"

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "CommonHtmlDialog.h"
#include "models/ExtractTargetSelectModel.h"
#include "utils/Utils.h"
#include "worker/AudioExtract.h"

using namespace AudioExtract;
using namespace utils;

WavExtractDialog::WavExtractDialog(QString srcWAVFileName, QString dstDirName,
                                   const std::optional<AudioIO::AudioFormat> &targetFormat, bool extractResultSelection,
                                   bool removeDCOffset, QWidget *parent)
    : QDialog(parent), m_srcWAVFileName(srcWAVFileName), m_dstDirName(dstDirName), m_targetFormat(targetFormat),
      m_extractResultSelection(extractResultSelection), m_removeDCOffset(removeDCOffset)
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
    connect(this, &WavExtractDialog::opened, this, &WavExtractDialog::startWork);
}

using PreCheckFutureWatcher = QFutureWatcher<decltype(std::function(preCheck))::result_type>;

void WavExtractDialog::startWork()
{
    auto nextFuture = QtConcurrent::run(preCheck, m_srcWAVFileName, m_dstDirName);
    m_label->setText(tr("Some preparing work..."));
    // This will make progress bar show as busy indicator
    m_progressBar->setMaximum(0);
    m_progressBar->setMinimum(0);
    auto watcher = new PreCheckFutureWatcher(this);
    watcher->setFuture(nextFuture);
    connect(watcher, &PreCheckFutureWatcher::finished, this, &WavExtractDialog::preCheckDone);
    connect(m_buttonBox, &QDialogButtonBox::rejected, watcher, &PreCheckFutureWatcher::cancel);
}

void WavExtractDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        switch (result.pass) {
        case AudioExtract::CheckPassType::CRITICAL: {
            CommonHtmlDialog dlg(this);
            dlg.setWindowTitle(tr("Error"));
            dlg.setLabel(tr("Critical error found. Can not continue."));
            dlg.setHTML(reportTextStyle + result.reportString);
            dlg.setStandardButtons(QDialogButtonBox::Ok);
            dlg.exec();
            done(QDialog::Rejected);
            break;
        }
        case AudioExtract::CheckPassType::WARNING: {
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
        case AudioExtract::CheckPassType::OK: {
            m_descRoot = result.descRoot;

            // Populate volume file list for multi-volume
            int volumeCount = m_descRoot["volume_count"].toInt(1);
            m_allVolumeFiles.clear();
            for (int i = 0; i < volumeCount; ++i)
                m_allVolumeFiles.append(utils::getVolumeFileName(m_srcWAVFileName, i));

            // Check for gap_duration (version 4+)
            m_gapDurationTimecode = m_descRoot.value("gap_duration").toString();
            if (!m_gapDurationTimecode.isEmpty() && m_gapDurationTimecode != QStringLiteral("00:00:00.000")) {
                QMessageBox gapBox(this);
                gapBox.setWindowTitle(tr("Space Between Entries"));
                gapBox.setText(tr("The combined file has %1 of silence padding on each side of every entry.\n"
                                  "How would you like to extract?")
                                   .arg(m_gapDurationTimecode));
                auto *originalBtn = gapBox.addButton(tr("Original range"), QMessageBox::AcceptRole);
                gapBox.addButton(tr("Include space"), QMessageBox::ActionRole);
                gapBox.setDefaultButton(originalBtn);
                gapBox.exec();
                if (gapBox.clickedButton() == originalBtn) {
                    m_extractGapMode = AudioExtract::ExtractGapMode::OriginalRange;
                } else {
                    m_extractGapMode = AudioExtract::ExtractGapMode::IncludeSpace;
                }
            }

            // Target selection works on JSON array directly (no audio data needed)
            QJsonArray descArray = m_descRoot["descriptions"].toArray();

            if (m_extractResultSelection) {
                auto selectDialog = new QDialog(this);
                auto selectModel = new ExtractTargetSelectModel(&descArray, selectDialog);
                auto selectDialogLayout = new QVBoxLayout(selectDialog);

                auto selectLabel = new QLabel(tr("Choose which ones to extract."), selectDialog);
                selectDialogLayout->addWidget(selectLabel);

                auto selectView = new QTableView(selectDialog);
                selectView->setModel(selectModel);
                selectView->resizeColumnsToContents();
                selectDialogLayout->addWidget(selectView);

                auto buttonLayout = new QHBoxLayout;
                auto selectAllButton = new QPushButton(tr("Select all"), selectDialog);
                auto unselectAllButton = new QPushButton(tr("Unselect all"), selectDialog);
                auto reverseSelectButton = new QPushButton(tr("Inverse"), selectDialog);
                buttonLayout->addWidget(selectAllButton);
                buttonLayout->addWidget(unselectAllButton);
                buttonLayout->addWidget(reverseSelectButton);
                buttonLayout->addStretch(1);
                connect(selectAllButton, &QPushButton::clicked, [selectModel]() { selectModel->selectAll(); });
                connect(unselectAllButton, &QPushButton::clicked, [selectModel]() { selectModel->unselectAll(); });
                connect(reverseSelectButton, &QPushButton::clicked,
                        [selectModel]() { selectModel->reverseSelect(); });
                selectDialogLayout->addLayout(buttonLayout);

                auto selectDialogButtonBox =
                    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, selectDialog);
                connect(selectDialogButtonBox, &QDialogButtonBox::accepted, selectDialog, &QDialog::accept);
                connect(selectDialogButtonBox, &QDialogButtonBox::rejected, selectDialog, &QDialog::reject);
                selectDialogLayout->addWidget(selectDialogButtonBox);

                selectDialog->setLayout(selectDialogLayout);
                selectDialog->resize(800, 500);

                if (selectDialog->exec() == QDialog::Rejected) {
                    done(QDialog::Rejected);
                    return;
                }
                // ExtractTargetSelectModel modifies the QJsonArray in-place (removes unchecked entries)
            }

            launchPipeline(descArray);
            break;
        }
        }
    }
}

void WavExtractDialog::launchPipeline(QJsonArray filteredDescArray)
{
    ExtractPipelineParams pipelineParams;
    pipelineParams.srcWAVFileName = m_srcWAVFileName;
    pipelineParams.descRoot = m_descRoot;
    pipelineParams.dstDirName = m_dstDirName;
    pipelineParams.targetFormat = m_targetFormat;
    pipelineParams.removeDCOffset = m_removeDCOffset;
    pipelineParams.gapMode = m_extractGapMode;
    pipelineParams.gapDurationTimecode = m_gapDurationTimecode;
    pipelineParams.filteredDescArray = filteredDescArray;

    // Set up progress tracking
    m_pipelineProgress = std::make_shared<std::atomic<int>>(0);
    m_tbbContext = std::make_shared<oneapi::tbb::task_group_context>();

    int totalEntries = filteredDescArray.size();
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
    auto nextFuture = QtConcurrent::run([pipelineParams, progressPtr, tbbCtx]() {
        return runExtractPipeline(pipelineParams, *progressPtr, *tbbCtx);
    });
    auto nextWatcher = new QFutureWatcher<ExtractPipelineResult>(this);
    nextWatcher->setFuture(nextFuture);
    connect(nextWatcher, &QFutureWatcher<ExtractPipelineResult>::finished, this, &WavExtractDialog::pipelineDone);
}

void WavExtractDialog::pipelineDone()
{
    // Stop progress polling
    if (m_progressTimer) {
        m_progressTimer->stop();
        m_progressTimer->deleteLater();
        m_progressTimer = nullptr;
    }

    if (auto watcher = dynamic_cast<QFutureWatcher<ExtractPipelineResult> *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future())) {
            done(QDialog::Rejected);
            return;
        }
        if (watcher->isCanceled() ||
            (m_tbbContext && m_tbbContext->is_group_execution_cancelled())) {
            done(QDialog::Rejected);
            return;
        }

        handlePipelineResult(watcher->result());
    }
}

void WavExtractDialog::handlePipelineResult(const ExtractPipelineResult &result)
{
    m_label->setText(tr("Done"));
    m_progressBar->setMaximum(1);
    m_progressBar->setMinimum(0);
    m_progressBar->setValue(1);

    if (result.errors.isEmpty()) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Icon::Information);
        msgBox.setText(tr("The wav file has been extracted."));
        msgBox.setInformativeText(tr("Extracted wav files has been stored at \"%1\"."
                                     "Original folder structure has been kept too.")
                                      .arg(m_dstDirName));
        auto deleteSrcButton = new QPushButton(tr("Delete source file"), &msgBox);
        connect(deleteSrcButton, &QPushButton::clicked, this, [this]() {
            bool allOk = true;
            for (const auto &volFile : std::as_const(m_allVolumeFiles)) {
                if (QFile::exists(volFile) && !QFile(volFile).remove()) {
                    QMessageBox::critical(this, {}, tr("Can not delete %1").arg(volFile));
                    allOk = false;
                }
            }
            auto removeDescFile = QFile(getDescFileNameFrom(m_srcWAVFileName)).remove();
            if (!removeDescFile) {
                QMessageBox::critical(this, {}, tr("Can not delete %1").arg(getDescFileNameFrom(m_srcWAVFileName)));
                allOk = false;
            }
            if (allOk)
                QMessageBox::information(this, {}, tr("Source files have been deleted successfully."));
        });
        msgBox.addButton(deleteSrcButton, QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        done(QDialog::Accepted);
    } else {
        // Show errors
        QString errorHtml = reportTextStyle;
        for (const auto &err : result.errors) {
            errorHtml.append(QString("<p class='critical'>%1</p>").arg(err.description));
        }

        QMessageBox msgBoxInformation;
        msgBoxInformation.setIcon(QMessageBox::Icon::Critical);
        msgBoxInformation.setText(tr("Error occurred when extracting."));
        msgBoxInformation.setInformativeText(errorHtml);
        msgBoxInformation.setStandardButtons(QMessageBox::Ok);
        msgBoxInformation.exec();

        // Offer retry
        QStringList failedFiles;
        QJsonArray retryDescArray;
        for (const auto &err : result.errors) {
            failedFiles.append(err.descObj.value("file_name").toString());
            retryDescArray.append(err.descObj);
        }

        QMessageBox msgBoxRetry;
        msgBoxRetry.setIcon(QMessageBox::Icon::Question);
        msgBoxRetry.setText(tr("Should retry extracting these?"));
        msgBoxRetry.setInformativeText(failedFiles.join("\n"));
        msgBoxRetry.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        auto retryDialogCode = msgBoxRetry.exec();
        if (retryDialogCode == QDialog::Rejected)
            done(QDialog::Rejected);
        else
            launchPipeline(retryDescArray);
    }
}

void WavExtractDialog::open()
{
    emit opened();
    QDialog::open();
}

int WavExtractDialog::exec()
{
    emit opened();
    return QDialog::exec();
}
