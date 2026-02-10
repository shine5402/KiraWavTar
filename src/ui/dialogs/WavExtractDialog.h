#ifndef WAVEXTRACTDIALOG_H
#define WAVEXTRACTDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>

#include "utils/Utils.h"
#include "worker/AudioIO.h"
#include "worker/WavExtract.h"

class QLabel;
class QProgressBar;
class QDialogButtonBox;

class WavExtractDialog : public QDialog
{
    Q_OBJECT

public:
    WavExtractDialog(QString srcWAVFileName, QString dstDirName, const AudioIO::AudioFormat &targetFormat,
                     bool extractResultSelection, bool removeDCOffset, QWidget *parent);

public slots:
    void open() override;
    int exec() override;

signals:
    void opened();

private:
    void doExtractCall(utils::AudioBufferPtr srcData, decltype(kfr::audio_format::samplerate) samplerate,
                       QJsonArray descArray);

private slots:
    void startWork();
    void preCheckDone();
    void readSrcAudioFileDone();
    void extractWorkDone();

private:
    QLabel *m_label;
    QProgressBar *m_progressBar;
    QDialogButtonBox *m_buttonBox;

    QString m_srcWAVFileName;
    QString m_dstDirName;
    AudioIO::AudioFormat m_targetFormat;
    bool m_extractResultSelection;
    bool m_removeDCOffset;
    AudioExtract::ExtractGapMode m_extractGapMode = AudioExtract::ExtractGapMode::OriginalRange;
    QString m_gapDurationTimecode;
    QStringList m_allVolumeFiles;
};

#endif // WAVEXTRACTDIALOG_H
