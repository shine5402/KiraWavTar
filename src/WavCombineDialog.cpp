#include "WavCombineDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QDialogButtonBox>
#include "kfr/all.hpp"
#include "WavCombine.h"
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QMessageBox>

using namespace WAVCombine;
using namespace wavtar_defines;

WAVCombineDialog::WAVCombineDialog(QString rootDirName, bool recursive,
                                   const AudioIO::WavAudioFormat& targetFormat, QString saveFileName, QWidget* parent)
    : QDialog(parent), rootDirName(rootDirName), recursive(recursive), targetFormat(targetFormat), saveFileName(saveFileName)
{
    auto layout = new QVBoxLayout(this);

    label = new QLabel(this);
    layout->addWidget(label);

    progressBar = new QProgressBar(this);
    progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(progressBar);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    setLayout(layout);
    resize(300, height());

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &WAVCombineDialog::opened, this, &WAVCombineDialog::startWork);
}

using PreCheckFutureWatcher = QFutureWatcher<decltype (std::function(preCheck))::result_type>;

void WAVCombineDialog::startWork()
{
    auto nextFuture = QtConcurrent::run(preCheck, rootDirName, saveFileName, recursive, targetFormat);
    label->setText(tr("Some preparing work..."));
    //This will make progress bar show as busy indicator
    progressBar->setMaximum(0);
    progressBar->setMinimum(0);
    auto watcher = new PreCheckFutureWatcher(this);
    watcher->setFuture(nextFuture);
    connect(watcher, &PreCheckFutureWatcher::finished, this, &WAVCombineDialog::preCheckDone);
    connect(buttonBox, &QDialogButtonBox::rejected, watcher, &PreCheckFutureWatcher::cancel);
}

using readAndCombineFutureWatcher = QFutureWatcher<QPair<std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t>>, QJsonObject>>;

void WAVCombineDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        QMessageBox msgBox;
        switch (result.pass) {
        case WAVCombine::CheckPassType::CRITICAL:
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("Critical error found. Can not continue."));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Rejected);
            break;
        case WAVCombine::CheckPassType::WARNING:
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("Some problems have been found but process can continue. Should we proceed?"));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
            if (msgBox.exec() == QMessageBox::No){
                this->done(QDialog::Rejected);
                this->close();
                return;
            }
            [[fallthrough]];
        case WAVCombine::CheckPassType::OK:
            auto nextFuture = startReadAndCombineWork(result.wavFileNames, rootDirName, targetFormat);
            auto nextWatcher = new readAndCombineFutureWatcher(this);
            nextWatcher->setFuture(nextFuture);
            label->setText(tr("Reading WAV files and combining them..."));
            progressBar->setMinimum(nextWatcher->progressMinimum());
            progressBar->setMaximum(nextWatcher->progressMaximum());
            connect(nextWatcher, &readAndCombineFutureWatcher::progressValueChanged, progressBar, &QProgressBar::setValue);
            connect(nextWatcher, &readAndCombineFutureWatcher::finished, this, &WAVCombineDialog::readAndCombineWorkDone);
            connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &readAndCombineFutureWatcher::cancel);
            break;
        }
    }
}

void WAVCombineDialog::readAndCombineWorkDone()
{
    if (auto watcher = dynamic_cast<readAndCombineFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        label->setText(tr("Writing combined file..."));
        //This will make progress bar show as busy indicator
        progressBar->setMaximum(0);
        progressBar->setMinimum(0);
        auto nextFuture = QtConcurrent::run(writeCombineResult, result.first, result.second, saveFileName, targetFormat);
        auto nextWatcher = new QFutureWatcher<bool>(this);
        nextWatcher->setFuture(nextFuture);
        connect(nextWatcher, &QFutureWatcher<bool>::finished, this, &WAVCombineDialog::writeResultDone);
        connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &QFutureWatcher<bool>::cancel);
    }
}

void WAVCombineDialog::writeResultDone()
{
    if (auto watcher = dynamic_cast<QFutureWatcher<bool>*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        label->setText(tr("Done"));
        progressBar->setMaximum(1);
        progressBar->setMinimum(0);
        progressBar->setValue(1);
        auto result = watcher->result();
        if (result){
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Icon::Information);
            msgBox.setText(tr("Wav files has been combined."));
            msgBox.setInformativeText(tr("Combined file has been stored at \"%1\"."
                                         "Please do not change the time when you edit it, "
                                         "and do not delete or modify \".kirawavtar-desc.json\" file sharing the same name with the WAV.")
                                      .arg(saveFileName));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Accepted);
        }
        else{
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

void WAVCombineDialog::open()
{
    emit opened();
    QDialog::open();
}

int WAVCombineDialog::exec()
{
    emit opened();
    return QDialog::exec();
}
