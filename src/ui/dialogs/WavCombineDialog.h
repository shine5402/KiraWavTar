#ifndef WAVCOMBINEDIALOG_H
#define WAVCOMBINEDIALOG_H

#include <QDialog>
#include <kfr/all.hpp>

#include "worker/AudioIO.h"

class QLabel;
class QProgressBar;
class QDialogButtonBox;

class WavCombineDialog : public QDialog
{
    Q_OBJECT
public:
    WavCombineDialog(QString rootDirName, bool recursive, const AudioIO::WavAudioFormat &targetFormat,
                     QString saveFileName, QWidget *parent = nullptr);

public slots:
    void open() override;
    int exec() override;

signals:
    void opened();

private slots:
    void startWork();
    void preCheckDone();
    void readAndCombineWorkDone();
    void writeResultDone();

private:
    QLabel *m_label;
    QProgressBar *m_progressBar;
    QDialogButtonBox *m_buttonBox;

    QString m_rootDirName;
    bool m_recursive;
    AudioIO::WavAudioFormat m_targetFormat;
    QString m_saveFileName;
};

#endif // WAVCOMBINEDIALOG_H
