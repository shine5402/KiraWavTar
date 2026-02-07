#include "WavExtractDialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "models/ExtractTargetSelectModel.h"
#include "utils/Utils.h"
#include "worker/WavExtract.h"

using namespace WAVExtract;
using namespace utils;

WavExtractDialog::WavExtractDialog(QString srcWAVFileName, QString dstDirName,
                                   const AudioIO::WavAudioFormat &targetFormat, bool extractResultSelection,
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

using ReadSrcWAVFileFutureWatcher = QFutureWatcher<decltype(std::function(readSrcWAVFile))::result_type>;

void WavExtractDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        QMessageBox msgBox;
        switch (result.pass) {
        case WAVExtract::CheckPassType::CRITICAL:
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("Critical error found. Can not continue."));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Rejected);
            break;
        case WAVExtract::CheckPassType::WARNING:
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("Some problems have been found but process can continue. Should we proceed?"));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            if (msgBox.exec() == QMessageBox::No) {
                this->done(QDialog::Rejected);
                this->close();
                return;
            }
            [[fallthrough]];
        case WAVExtract::CheckPassType::OK:
            auto nextFuture = QtConcurrent::run(readSrcWAVFile, m_srcWAVFileName, result.descRoot, m_targetFormat);
            auto nextWatcher = new ReadSrcWAVFileFutureWatcher(this);
            nextWatcher->setFuture(nextFuture);
            m_label->setText(tr("Reading source WAV file..."));
            connect(nextWatcher, &ReadSrcWAVFileFutureWatcher::finished, this, &WavExtractDialog::readSrcWAVFileDone);
            connect(m_buttonBox, &QDialogButtonBox::rejected, nextWatcher, &ReadSrcWAVFileFutureWatcher::cancel);
            break;
        }
    }
}

using ExtractWorkFutureWatcher = QFutureWatcher<ExtractErrorDescription>;

// Though use SrcData as param would be better, but it will expose this internal struct in wavextractdialog.h, so..
void WavExtractDialog::doExtractCall(utils::AudioBufferPtr srcData, decltype(kfr::audio_format::samplerate) samplerate,
                                     QJsonArray descArray)
{
    m_label->setText(tr("Writing extracted file..."));
    auto nextFuture = startExtract(srcData, samplerate, descArray, m_dstDirName, m_targetFormat, m_removeDCOffset);
    auto nextWatcher = new ExtractWorkFutureWatcher(this);
    nextWatcher->setFuture(nextFuture);
    m_progressBar->setMinimum(nextWatcher->progressMinimum());
    m_progressBar->setMaximum(nextWatcher->progressMaximum());
    connect(nextWatcher, &QFutureWatcher<bool>::progressValueChanged, m_progressBar, &QProgressBar::setValue);
    connect(nextWatcher, &QFutureWatcher<bool>::finished, this, &WavExtractDialog::extractWorkDone);
    connect(m_buttonBox, &QDialogButtonBox::rejected, nextWatcher, &QFutureWatcher<bool>::cancel);
}

void WavExtractDialog::readSrcWAVFileDone()
{
    if (auto watcher = dynamic_cast<ReadSrcWAVFileFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        if (m_extractResultSelection) {
            auto selectDialog = new QDialog(this);
            auto selectModel = new ExtractTargetSelectModel(&result.descArray, selectDialog);
            auto selectDialogLayout = new QVBoxLayout(selectDialog);

            auto selectLabel = new QLabel(tr("Choose which ones to extract."), selectDialog);
            selectDialogLayout->addWidget(selectLabel);

            auto selectView = new QTableView(selectDialog);
            selectView->setModel(selectModel);
            selectView->resizeColumnsToContents(); // FILENAME
            // selectView->horizontalHeader()->setStretchLastSection(true);
            // selectView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);//0 for FILENAME
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
            connect(reverseSelectButton, &QPushButton::clicked, [selectModel]() { selectModel->reverseSelect(); });
            selectDialogLayout->addLayout(buttonLayout);

            auto selectDialogButtonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, selectDialog);
            connect(selectDialogButtonBox, &QDialogButtonBox::accepted, selectDialog, &QDialog::accept);
            connect(selectDialogButtonBox, &QDialogButtonBox::rejected, selectDialog, &QDialog::reject);
            selectDialogLayout->addWidget(selectDialogButtonBox);

            selectDialog->setLayout(selectDialogLayout);

            selectDialog->resize(800, 500);
            auto selectDialogResult = selectDialog->exec();
            if (selectDialogResult == QDialog::Rejected) {
                done(QDialog::Rejected);
                return;
            }
            // As model takes a pointer to result.second and modify it directly, there is no need for us to do extra
            // work here.
        }
        doExtractCall(result.srcData, result.samplerate, result.descArray);
    }
}

void WavExtractDialog::extractWorkDone()
{
    if (auto watcher = dynamic_cast<ExtractWorkFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        m_label->setText(tr("Done"));
        // Set progressbar as done
        m_progressBar->setMaximum(1);
        m_progressBar->setMinimum(0);
        m_progressBar->setValue(1);
        // We accumulate errors
        auto allResults = watcher->future().results();
        QList<ExtractErrorDescription> errors;
        for (const auto &res : allResults) {
            if (!res.description.isEmpty())
                errors.append(res);
        }

        if (errors.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Icon::Information);
            msgBox.setText(tr("The wav file has been extracted."));
            msgBox.setInformativeText(tr("Extracted wav files has been stored at \"%1\"."
                                         "Original folder structure has been kept too.")
                                          .arg(m_dstDirName));
            auto deleteSrcButton = new QPushButton(tr("Delete source file"), &msgBox);
            connect(deleteSrcButton, &QPushButton::clicked, this, [this]() {
                auto removeSrcWAV = QFile(m_srcWAVFileName).remove();
                auto removeDescFile = QFile(getDescFileNameFrom(m_srcWAVFileName)).remove();
                if (!removeSrcWAV)
                    QMessageBox::critical(this, {}, tr("Can not delete %1").arg(m_srcWAVFileName));
                if (!removeDescFile)
                    QMessageBox::critical(this, {}, tr("Can not delete %1").arg(getDescFileNameFrom(m_srcWAVFileName)));
                if (removeSrcWAV && removeDescFile)
                    QMessageBox::information(this, {}, tr("Source files have been deleted successfully."));
            });
            msgBox.addButton(deleteSrcButton, QMessageBox::AcceptRole);
            msgBox.addButton(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Accepted);
        } else {
            QMessageBox msgBoxInfomation;
            msgBoxInfomation.setIcon(QMessageBox::Icon::Critical);
            msgBoxInfomation.setText(tr("Error occurred when extracting."));
            msgBoxInfomation.setInformativeText(
                QtConcurrent::mappedReduced<QString>(
                    errors,
                    std::function([](const ExtractErrorDescription &value) -> QString { return value.description; }),
                    [](QString &res, const QString &desc) {
                        if (res.isEmpty())
                            res = reportTextStyle;
                        res.append(QString("<p class='critical'>%1</p>").arg(desc));
                    })
                    .result());
            msgBoxInfomation.setStandardButtons(QMessageBox::Ok);
            msgBoxInfomation.exec();
            QMessageBox msgBoxRetry;
            msgBoxRetry.setIcon(QMessageBox::Icon::Question);
            msgBoxRetry.setText(tr("Should retry extracting these?"));
            msgBoxRetry.setInformativeText(QtConcurrent::mappedReduced<QStringList>(
                                               errors,
                                               std::function([](const ExtractErrorDescription &value) -> QString {
                                                   return value.descObj.value("file_name").toString();
                                               }),
                                               [](QStringList &res, const QString &fileName) { res.append(fileName); })
                                               .result()
                                               .join("\n"));
            msgBoxRetry.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            auto retryDialogCode = msgBoxRetry.exec();
            if (retryDialogCode == QDialog::Rejected)
                done(QDialog::Rejected);
            else {
                QJsonArray descArray;
                for (const auto &i : std::as_const(errors)) {
                    descArray.append(i.descObj);
                }
                auto srcData = errors.at(0).srcData;
                auto srcSampleRate = errors.at(0).srcSampleRate;
                doExtractCall(srcData, srcSampleRate, descArray);
            }
        }
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
