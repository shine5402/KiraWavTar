#include "wavextractdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QDialogButtonBox>
#include "wavextract.h"
#include <QtConcurrent>
#include <QMessageBox>
#include "extracttargetselectmodel.h"
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>

using namespace WAVExtract;
using namespace wavtar_defines;

WAVExtractDialog::WAVExtractDialog(QString srcWAVFileName, QString dstDirName,
                                   const kfr::audio_format& targetFormat, bool extractResultSelection, QWidget* parent)
    : QDialog(parent), srcWAVFileName(srcWAVFileName), dstDirName(dstDirName), targetFormat(targetFormat), extractResultSelection(extractResultSelection)
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
            done(QDialog::Rejected);
            break;
        case WAVExtract::CheckPassType::WARNING:
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("发现了一些问题，不过操作仍然可以继续。请问要继续吗？"));
            msgBox.setInformativeText(reportTextStyle + result.reportString);
            msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
            if (msgBox.exec() == QMessageBox::No){
                this->done(QDialog::Rejected);
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

void WAVExtractDialog::doExtractCall(std::shared_ptr<kfr::univector2d<sample_process_t>> srcData, QJsonArray descArray)
{
    label->setText(tr("拆分波形文件并写入……"));
    auto nextFuture = startExtract(srcData, descArray, dstDirName, targetFormat);
    auto nextWatcher = new ExtractWorkFutureWatcher(this);
    nextWatcher->setFuture(nextFuture);
    progressBar->setMinimum(nextWatcher->progressMinimum());
    progressBar->setMaximum(nextWatcher->progressMaximum());
    connect(nextWatcher, &QFutureWatcher<bool>::progressValueChanged, progressBar, &QProgressBar::setValue);
    connect(nextWatcher, &QFutureWatcher<bool>::finished, this, &WAVExtractDialog::extractWorkDone);
    connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &QFutureWatcher<bool>::cancel);
}

void WAVExtractDialog::readSrcWAVFileDone()
{
    if (auto watcher = dynamic_cast<ReadSrcWAVFileFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        auto result = watcher->result();
        if (extractResultSelection)
        {
            auto selectDialog = new QDialog(this);
            auto selectModel = new ExtractTargetSelectModel(&result.second, selectDialog);
            auto selectDialogLayout = new QVBoxLayout(selectDialog);

            auto selectLabel = new QLabel(tr("请选择要被拆分的项。"), selectDialog);
            selectDialogLayout->addWidget(selectLabel);

            auto selectView = new QTableView(selectDialog);
            selectView->setModel(selectModel);
            //selectView->resizeColumnToContents(0);//FILENAME
            selectView->horizontalHeader()->setStretchLastSection(false);
            selectView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);//0 for FILENAME
            selectDialogLayout->addWidget(selectView);

            auto buttonLayout = new QHBoxLayout(selectDialog);
            auto selectAllButton = new QPushButton(tr("选择全部"), selectDialog);
            auto unselectAllButton = new QPushButton(tr("取消选择全部"), selectDialog);
            auto reverseSelectButton = new QPushButton(tr("选择反向"), selectDialog);
            buttonLayout->addWidget(selectAllButton);
            buttonLayout->addWidget(unselectAllButton);
            buttonLayout->addWidget(reverseSelectButton);
            buttonLayout->addStretch(1);
            connect(selectAllButton, &QPushButton::clicked, [selectModel](){selectModel->selectAll();});
            connect(unselectAllButton, &QPushButton::clicked, [selectModel](){selectModel->unselectAll();});
            connect(reverseSelectButton, &QPushButton::clicked, [selectModel](){selectModel->reverseSelect();});
            selectDialogLayout->addLayout(buttonLayout);

            auto selectDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, selectDialog);
            connect(selectDialogButtonBox, &QDialogButtonBox::accepted, selectDialog, &QDialog::accept);
            connect(selectDialogButtonBox, &QDialogButtonBox::rejected, selectDialog, &QDialog::reject);
            selectDialogLayout->addWidget(selectDialogButtonBox);

            selectDialog->setLayout(selectDialogLayout);

            selectDialog->resize(800, 500);
            auto selectDialogResult = selectDialog->exec();
            if (selectDialogResult == QDialog::Rejected){
                done(QDialog::Rejected);
                return;
            }
            //As model takes a pointer to result.second and modify it directly, there is no need for us to do extra work here.
        }
        doExtractCall(result.first, result.second);
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
            done(QDialog::Accepted);
        }
        else{
            QMessageBox msgBoxInfomation;
            msgBoxInfomation.setIcon(QMessageBox::Icon::Critical);
            msgBoxInfomation.setText(tr("拆分波形文件时出现问题。"));
            msgBoxInfomation.setInformativeText(QtConcurrent::mappedReduced<QString>(result,
                                                                  std::function([](const ExtractErrorDescription& value)->QString{return value.description;}),
            [](QString& result, const QString& desc){
                                          if (result.isEmpty())
                                            result = reportTextStyle;
                                          result.append(QString("<p class='critical'>%1</p>").arg(desc));
                                      }));
            msgBoxInfomation.setStandardButtons(QMessageBox::Ok);
            msgBoxInfomation.exec();
            //TODO:ask user retry errored files here
            QMessageBox msgBoxRetry;
            msgBoxRetry.setIcon(QMessageBox::Icon::Question);
            msgBoxRetry.setText(tr("要重试拆分这些文件吗？"));
            msgBoxRetry.setInformativeText(QtConcurrent::mappedReduced<QStringList>(result, std::function([](const ExtractErrorDescription& value)->QString{return value.descObj.value("file_name").toString();}),
                                                                                     [](QStringList& result, const QString& fileName){
                                                    result.append(fileName);
                                                }).result().join("\n"));
            msgBoxRetry.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            auto retryDialogCode = msgBoxRetry.exec();
            if (retryDialogCode == QDialog::Rejected)
                done(QDialog::Rejected);
            else
            {
                QJsonArray descArray;
                for (const auto& i : std::as_const(result)){
                    descArray.append(i.descObj);
                }
                auto srcData = result.at(0).srcData;
                doExtractCall(srcData, descArray);
            }
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
