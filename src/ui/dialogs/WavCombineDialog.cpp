#include "WavCombineDialog.h"
#include "CommonHtmlDialog.h"

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

#include "kfr/all.hpp"
#include "worker/WavCombine.h"

using namespace WAVCombine;
using namespace utils;

WavCombineDialog::WavCombineDialog(QString rootDirName, bool recursive, const AudioIO::WavAudioFormat &targetFormat,
                                   QString saveFileName, QWidget *parent)
    : QDialog(parent), m_rootDirName(rootDirName), m_recursive(recursive), m_targetFormat(targetFormat),
      m_saveFileName(saveFileName)
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

using readAndCombineFutureWatcher = QFutureWatcher<QPair<utils::AudioBufferPtr, QJsonObject>>;

void WavCombineDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        switch (result.pass) {
        case WAVCombine::CheckPassType::CRITICAL: {
            CommonHtmlDialog dlg(this);
            dlg.setWindowTitle(tr("Error"));
            dlg.setLabel(tr("Critical error found. Can not continue."));
            dlg.setHTML(reportTextStyle + result.reportString);
            dlg.setStandardButtons(QDialogButtonBox::Ok);
            dlg.exec();
            done(QDialog::Rejected);
            break;
        }
        case WAVCombine::CheckPassType::WARNING: {
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
        case WAVCombine::CheckPassType::OK:
            auto nextFuture = startReadAndCombineWork(result.wavFileNames, m_rootDirName, m_targetFormat);
            auto nextWatcher = new readAndCombineFutureWatcher(this);
            nextWatcher->setFuture(nextFuture);
            m_label->setText(tr("Reading WAV files and combining them..."));
            m_progressBar->setMinimum(nextWatcher->progressMinimum());
            m_progressBar->setMaximum(nextWatcher->progressMaximum());
            connect(nextWatcher, &readAndCombineFutureWatcher::progressValueChanged, m_progressBar,
                    &QProgressBar::setValue);
            connect(nextWatcher, &readAndCombineFutureWatcher::finished, this,
                    &WavCombineDialog::readAndCombineWorkDone);
            connect(m_buttonBox, &QDialogButtonBox::rejected, nextWatcher, &readAndCombineFutureWatcher::cancel);
            break;
        }
    }
}

void WavCombineDialog::readAndCombineWorkDone()
{
    if (auto watcher = dynamic_cast<readAndCombineFutureWatcher *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        m_label->setText(tr("Writing combined file..."));
        // This will make progress bar show as busy indicator
        m_progressBar->setMaximum(0);
        m_progressBar->setMinimum(0);
        auto nextFuture =
            QtConcurrent::run(writeCombineResult, result.first, result.second, m_saveFileName, m_targetFormat);
        auto nextWatcher = new QFutureWatcher<bool>(this);
        nextWatcher->setFuture(nextFuture);
        connect(nextWatcher, &QFutureWatcher<bool>::finished, this, &WavCombineDialog::writeResultDone);
        connect(m_buttonBox, &QDialogButtonBox::rejected, nextWatcher, &QFutureWatcher<bool>::cancel);
    }
}

void WavCombineDialog::writeResultDone()
{
    if (auto watcher = dynamic_cast<QFutureWatcher<bool> *>(QObject::sender())) {
        if (!utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        m_label->setText(tr("Done"));
        m_progressBar->setMaximum(1);
        m_progressBar->setMinimum(0);
        m_progressBar->setValue(1);
        auto result = watcher->result();
        if (result) {
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
        } else {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.setText(tr("Error occurred when writing combined WAV."));
            msgBox.setInformativeText(tr("Please check potential causes and try again."));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Rejected);
        }
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
