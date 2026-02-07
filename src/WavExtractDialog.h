#ifndef WAVEXTRACTDIALOG_H
#define WAVEXTRACTDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>
#include "WavTarUtils.h"
#include "AudioIO.h"

class QLabel;
class QProgressBar;
class QDialogButtonBox;

class WAVExtractDialog : public QDialog
{
    Q_OBJECT
public:
    WAVExtractDialog(QString srcWAVFileName, QString dstDirName, const AudioIO::WavAudioFormat& targetFormat, bool extractResultSelection, bool removeDCOffset, QWidget* parent);
private:
    QLabel* label;
    QProgressBar* progressBar;
    QDialogButtonBox* buttonBox;

    QString srcWAVFileName;
    QString dstDirName;
    AudioIO::WavAudioFormat targetFormat;
    bool extractResultSelection;
    bool removeDCOffset;

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
