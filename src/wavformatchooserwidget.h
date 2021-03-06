#ifndef WAVFORMATCHOOSERWIDGET_H
#define WAVFORMATCHOOSERWIDGET_H

#include <QWidget>
#include <kfr/all.hpp>

namespace Ui {
    class WAVFormatChooserWidget;
}

class WAVFormatChooserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WAVFormatChooserWidget(QWidget *parent = nullptr);
    ~WAVFormatChooserWidget();

    decltype (kfr::audio_format::samplerate) getSampleRate() const;
    decltype (kfr::audio_format::channels) getChannelCount() const;
    decltype (kfr::audio_format::type) getSampleType() const;
    decltype (kfr::audio_format::wav_format) getWAVContainerFormat() const;

    void setSampleRate(decltype(kfr::audio_format::samplerate) value);
    void setChannelCount(decltype(kfr::audio_format::channels) value);
    void setSampleType(decltype(kfr::audio_format::type) value);
    void setWAVContainerFormat(decltype(kfr::audio_format::wav_format) value);

    kfr::audio_format getFormat() const;
    void setFormat(kfr::audio_format value);

    void reset();
private:
    Ui::WAVFormatChooserWidget *ui;

private slots:
    void warnAbout64Bit();
protected:
    void changeEvent(QEvent* event) override;
};

#endif // WAVFORMATCHOOSERWIDGET_H
