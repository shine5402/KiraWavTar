#ifndef WAVFORMATCHOOSERWIDGET_H
#define WAVFORMATCHOOSERWIDGET_H

#include <QWidget>
#include <kfr/all.hpp>

#include "worker/AudioIO.h"

namespace Ui {
class WAVFormatChooserWidget;
}

class WAVFormatChooserWidget : public QWidget
{
    Q_OBJECT

public:
    // Special index values for the channels combobox
    static constexpr int ChannelIndexAuto = 0;
    static constexpr int ChannelIndexCustom = -1; // Will be set dynamically after separator

    // Mode for the auto channel option text
    enum class AutoChannelMode {
        MaxFromInput,    // "Auto (max from input)" - for combine mode
        InheritFromInput // "Inherit from input" - for extract mode
    };

    explicit WAVFormatChooserWidget(QWidget *parent = nullptr);
    ~WAVFormatChooserWidget();

    decltype(kfr::audio_format::samplerate) getSampleRate() const;
    decltype(kfr::audio_format::channels) getChannelCount() const;
    decltype(kfr::audio_format::type) getSampleType() const;
    AudioIO::WavAudioFormat::Container getWAVContainerFormat() const;

    AudioIO::WavAudioFormat getFormat() const;

    bool isAutoChannelCount() const;
    void setAutoChannelCountValue(int value);
    void setAutoChannelMode(AutoChannelMode mode);

    void reset();

public slots:
    void showFormatHelp();

private slots:
    void onChannelsComboBoxChanged(int index);

private:
    void setupChannelsComboBox();

    Ui::WAVFormatChooserWidget *ui;
    int m_customChannelIndex = -1; // Index of "Custom..." option
    int m_autoChannelValue = 2;    // Default auto value
    AutoChannelMode m_autoChannelMode = AutoChannelMode::MaxFromInput;
};

#endif // WAVFORMATCHOOSERWIDGET_H
