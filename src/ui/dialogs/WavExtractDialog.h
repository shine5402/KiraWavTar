#ifndef WAVEXTRACTDIALOG_H
#define WAVEXTRACTDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>

#include "utils/Utils.h"
#include "worker/AudioIO.h"

class QLabel;
class QProgressBar;
class QDialogButtonBox;

class WavExtractDialog : public QDialog
{
    Q_OBJECT

public:
    WavExtractDialog(QString srcWAVFileName, QString dstDirName, const AudioIO::WavAudioFormat &targetFormat,
                     bool extractResultSelection, bool removeDCOffset, QWidget *parent);

private:
    QLabel *label;
    QProgressBar *progressBar;
    QDialogButtonBox *buttonBox;

    QString srcWAVFileName;
    QString dstDirName;
    AudioIO::WavAudioFormat targetFormat;
    bool extractResultSelection;
    bool removeDCOffset;

    void doExtractCall(std::shared_ptr<kfr::univector2d<utils::sample_process_t>> srcData,
                       decltype(kfr::audio_format::samplerate) samplerate, QJsonArray descArray);

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
