#ifndef WAVFORMATCHOOSERWIDGET_H
#define WAVFORMATCHOOSERWIDGET_H

#include <QWidget>
#include <kfr/all.hpp>

#include "AudioIO.h"

namespace Ui {
class WAVFormatChooserWidget;
}

class WAVFormatChooserWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit WAVFormatChooserWidget(QWidget *parent = nullptr);
    ~WAVFormatChooserWidget();

    decltype(kfr::audio_format::samplerate) getSampleRate() const;
    decltype(kfr::audio_format::channels) getChannelCount() const;
    decltype(kfr::audio_format::type) getSampleType() const;
    AudioIO::WavAudioFormat::Container getWAVContainerFormat() const;

    AudioIO::WavAudioFormat getFormat() const;

    void reset();

  public slots:
    void warnAbout64Bit(bool checked);

  private:
    Ui::WAVFormatChooserWidget *ui;
};

#endif // WAVFORMATCHOOSERWIDGET_H
