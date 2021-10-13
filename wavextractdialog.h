#ifndef WAVEXTRACTDIALOG_H
#define WAVEXTRACTDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>
#include "wavtar_utils.h"

class QLabel;
class QProgressBar;
class QDialogButtonBox;

class WAVExtractDialog : public QDialog
{
    Q_OBJECT
public:
    WAVExtractDialog(QString srcWAVFileName, QString dstDirName, const kfr::audio_format& targetFormat, bool extractResultSelection, QWidget* parent);
private:
    QLabel* label;
    QProgressBar* progressBar;
    QDialogButtonBox* buttonBox;

    QString srcWAVFileName;
    QString dstDirName;
    kfr::audio_format targetFormat;
    bool extractResultSelection;

    void doExtractCall(std::shared_ptr<kfr::univector2d<wavtar_defines::sample_process_t> > srcData, decltype(kfr::audio_format::samplerate) samplerate, QJsonArray descArray);

private slots:
    void startWork();
    void preCheckDone();
    void readSrcWAVFileDone();
    void extractWorkDone();

signals:
    void opened();

public slots:
    void open() override;
    int exec() override;

};

#endif // WAVEXTRACTDIALOG_H
