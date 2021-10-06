#include "wavextractdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QDialogButtonBox>
#include "wavextract.h"
#include <QtConcurrent>
#include <QMessageBox>

using namespace WAVExtract;
using namespace wavtar_defines;

WAVExtractDialog::WAVExtractDialog(QString srcWAVFileName, QString dstDirName,
                                   const kfr::audio_format& targetFormat, QWidget* parent)
    : QDialog(parent), srcWAVFileName(srcWAVFileName), dstDirName(dstDirName), targetFormat(targetFormat)
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
    connect(this, &WAVExtractDialog::opened, this, &WAVExtractDialog::startWork);
}

using PreCheckFutureWatcher = QFutureWatcher<decltype (std::function(preCheck))::result_type>;

void WAVExtractDialog::startWork()
{
    auto nextFuture = QtConcurrent::run(preCheck, srcWAVFileName, dstDirName);
    label->setText(tr("一些准备工作……"));
    //This will make progress bar show as busy indicator
    progressBar->setMaximum(0);
    progressBar->setMinimum(0);
    auto watcher = new PreCheckFutureWatcher(this);
    watcher->setFuture(nextFuture);
    connect(watcher, &PreCheckFutureWatcher::finished, this, &WAVExtractDialog::preCheckDone);
    connect(buttonBox, &QDialogButtonBox::rejected, watcher, &PreCheckFutureWatcher::cancel);
}

using ReadSrcWAVFileFutureWatcher = QFutureWatcher<decltype (std::function(readSrcWAVFile))::result_type>;

void WAVExtractDialog::preCheckDone()
{
    if (auto watcher = dynamic_cast<PreCheckFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        QMessageBox msgBox;
        switch (result.pass) {
        case WAVExtract::CheckPassType::CRITICAL:
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("发现了一些严重问题，操作无法继续。"));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            reject();
            break;
        case WAVExtract::CheckPassType::WARNING:
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("发现了一些问题，不过操作仍然可以继续。请问要继续吗？"));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
            if (msgBox.exec() == QMessageBox::No){
                this->reject();
                this->close();
                return;
            }
            [[fallthrough]];
        case WAVExtract::CheckPassType::OK:
            auto nextFuture = QtConcurrent::run(readSrcWAVFile, srcWAVFileName, result.descRoot);
            auto nextWatcher = new ReadSrcWAVFileFutureWatcher(this);
            nextWatcher->setFuture(nextFuture);
            label->setText(tr("读取源波形文件……"));
            connect(nextWatcher, &ReadSrcWAVFileFutureWatcher::finished, this, &WAVExtractDialog::readSrcWAVFileDone);
            connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &ReadSrcWAVFileFutureWatcher::cancel);
            break;
        }
    }
}

using ExtractWorkFutureWatcher = QFutureWatcher<QList<ExtractErrorDescription>>;

void WAVExtractDialog::readSrcWAVFileDone()
{
    if (auto watcher = dynamic_cast<ReadSrcWAVFileFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        label->setText(tr("拆分波形文件并写入……"));
        auto nextFuture = startExtract(result.first, result.second, dstDirName, targetFormat);
        auto nextWatcher = new ExtractWorkFutureWatcher(this);
        nextWatcher->setFuture(nextFuture);
        progressBar->setMinimum(nextWatcher->progressMinimum());
        progressBar->setMaximum(nextWatcher->progressMaximum());
        connect(nextWatcher, &QFutureWatcher<bool>::progressValueChanged, progressBar, &QProgressBar::setValue);
        connect(nextWatcher, &QFutureWatcher<bool>::finished, this, &WAVExtractDialog::extractWorkDone);
        connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &QFutureWatcher<bool>::cancel);
    }
}

void WAVExtractDialog::extractWorkDone()
{
    if (auto watcher = dynamic_cast<ExtractWorkFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        label->setText(tr("完成"));
        progressBar->setMaximum(1);
        progressBar->setMinimum(0);
        progressBar->setValue(1);
        auto result = watcher->result();
        if (result.isEmpty()){
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Icon::Information);
            msgBox.setText(tr("拆分操作已经完成。"));
            msgBox.setInformativeText(tr("拆分后的波形文件已经存储至%1，原先的文件夹结构也已经被还原（如果有）。").arg(dstDirName));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            accept();
        }
        else{
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.setText(tr("拆分波形文件时出现问题。"));
            msgBox.setInformativeText(QtConcurrent::mappedReduced<QString>(result,
                                                                  std::function([](const ExtractErrorDescription& value)->QString{return value.description;}),
            [](QString& result, const QString& desc){
                                          if (result.isEmpty())
                                            result = reportTextStyle;
                                          result.append(QString("<p class='critical'>%1</p>").arg(desc));
                                      }));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            reject();
        }
    }
}

void WAVExtractDialog::open()
{
    emit opened();
    QDialog::open();
}

int WAVExtractDialog::exec()
{
    emit opened();
    QDialog::exec();
}
