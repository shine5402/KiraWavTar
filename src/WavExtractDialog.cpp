#include "WavExtractDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QDialogButtonBox>
#include "WavExtract.h"
#include <QtConcurrent>
#include <QMessageBox>
#include "ExtractTargetSelectModel.h"
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>
#include "WavTarUtils.h"

using namespace WAVExtract;
using namespace wavtar_defines;
using namespace wavtar_utils;

WAVExtractDialog::WAVExtractDialog(QString srcWAVFileName, QString dstDirName,
                                   const kfr::audio_format& targetFormat, bool extractResultSelection, bool removeDCOffset, QWidget* parent)
    : QDialog(parent), srcWAVFileName(srcWAVFileName), dstDirName(dstDirName), targetFormat(targetFormat), extractResultSelection(extractResultSelection), removeDCOffset(removeDCOffset)
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
    label->setText(tr("Some preparing work..."));
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
            label->setText(tr("Reading source WAV file..."));
            connect(nextWatcher, &ReadSrcWAVFileFutureWatcher::finished, this, &WAVExtractDialog::readSrcWAVFileDone);
            connect(buttonBox, &QDialogButtonBox::rejected, nextWatcher, &ReadSrcWAVFileFutureWatcher::cancel);
            break;
        }
    }
}

using ExtractWorkFutureWatcher = QFutureWatcher<QList<ExtractErrorDescription>>;

// Though use SrcData as param would be better, but it will expose this internal struct in wavextractdialog.h, so..
void WAVExtractDialog::doExtractCall(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t> > srcData, decltype(kfr::audio_format::samplerate) samplerate, QJsonArray descArray)
{
    label->setText(tr("Writing extracted file..."));
    auto nextFuture = startExtract(srcData, samplerate, descArray, dstDirName, targetFormat, removeDCOffset);
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
            auto selectModel = new ExtractTargetSelectModel(&result.descArray, selectDialog);
            auto selectDialogLayout = new QVBoxLayout(selectDialog);

            auto selectLabel = new QLabel(tr("Choose which ones to extract."), selectDialog);
            selectDialogLayout->addWidget(selectLabel);

            auto selectView = new QTableView(selectDialog);
            selectView->setModel(selectModel);
            selectView->resizeColumnsToContents();//FILENAME
            //selectView->horizontalHeader()->setStretchLastSection(true);
            //selectView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);//0 for FILENAME
            selectDialogLayout->addWidget(selectView);

            auto buttonLayout = new QHBoxLayout;
            auto selectAllButton = new QPushButton(tr("Select all"), selectDialog);
            auto unselectAllButton = new QPushButton(tr("Unselect all"), selectDialog);
            auto reverseSelectButton = new QPushButton(tr("Inverse"), selectDialog);
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
        doExtractCall(result.srcData, result.samplerate, result.descArray);
    }
}

void WAVExtractDialog::extractWorkDone()
{
    if (auto watcher = dynamic_cast<ExtractWorkFutureWatcher*>(QObject::sender())){
        if (!wavtar_utils::checkFutureExceptionAndWarn(watcher->future()))
            return;
        if (watcher->isCanceled())
            return;
        label->setText(tr("Done"));
        //Set progressbar as done
        progressBar->setMaximum(1);
        progressBar->setMinimum(0);
        progressBar->setValue(1);
        auto result = watcher->result();
        if (result.isEmpty()){
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Icon::Information);
            msgBox.setText(tr("The wav file has been extracted."));
            msgBox.setInformativeText(tr("Extracted wav files has been stored at \"%1\"."
                                         "Original folder structure has been kept too.")
                                      .arg(dstDirName));
            auto deleteSrcButton = new QPushButton(tr("Delete source file"), &msgBox);
            connect(deleteSrcButton, &QPushButton::clicked, this, [this](){
                auto removeSrcWAV = QFile(srcWAVFileName).remove();
                auto removeDescFile = QFile(getDescFileNameFrom(srcWAVFileName)).remove();
                 if (!removeSrcWAV)
                     QMessageBox::critical(this, {}, tr("Can not delete %1").arg(srcWAVFileName));
                 if (!removeDescFile)
                     QMessageBox::critical(this, {}, tr("Can not delete %1").arg(getDescFileNameFrom(srcWAVFileName)));
                 if (removeSrcWAV && removeDescFile)
                     QMessageBox::information(this, {}, tr("Source files have been deleted successfully."));

            });
            msgBox.addButton(deleteSrcButton, QMessageBox::AcceptRole);
            msgBox.addButton(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            done(QDialog::Accepted);
        }
        else{
            QMessageBox msgBoxInfomation;
            msgBoxInfomation.setIcon(QMessageBox::Icon::Critical);
            msgBoxInfomation.setText(tr("Error occurred when extracting."));
            msgBoxInfomation.setInformativeText(QtConcurrent::mappedReduced<QString>(result,
                                                                  std::function([](const ExtractErrorDescription& value)->QString{return value.description;}),
            [](QString& result, const QString& desc){
                                          if (result.isEmpty())
                                            result = reportTextStyle;
                                          result.append(QString("<p class='critical'>%1</p>").arg(desc));
                                      }).result());
            msgBoxInfomation.setStandardButtons(QMessageBox::Ok);
            msgBoxInfomation.exec();
            QMessageBox msgBoxRetry;
            msgBoxRetry.setIcon(QMessageBox::Icon::Question);
            msgBoxRetry.setText(tr("Should retry extracting these?"));
            msgBoxRetry.setInformativeText(QtConcurrent::mappedReduced<QStringList>(result,
                                                                                    std::function([](const ExtractErrorDescription& value)->QString{return value.descObj.value("file_name").toString();}),
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
                auto srcSampleRate = result.at(0).srcSampleRate;
                doExtractCall(srcData, srcSampleRate, descArray);
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
    return QDialog::exec();
}
