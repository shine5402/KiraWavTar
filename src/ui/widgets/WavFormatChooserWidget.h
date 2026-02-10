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
    // Special index values for the comboboxes (auto option is always index 0)
    static constexpr int AutoIndex = 0;
    static constexpr int ChannelIndexAuto = AutoIndex; // Legacy alias
    static constexpr int ChannelIndexCustom = -1;      // Will be set dynamically after separator

    // Mode for the auto option text (applies to sample rate, bit depth, and channels)
    enum class AutoMode {
        MaxFromInput,    // "Auto (max from input)" - for combine mode
        InheritFromInput // "Inherit from input" - for extract mode
    };

    // Legacy alias for compatibility
    using AutoChannelMode = AutoMode;

    explicit WAVFormatChooserWidget(QWidget *parent = nullptr);
    ~WAVFormatChooserWidget();

    decltype(kfr::audio_format::samplerate) getSampleRate() const;
    decltype(kfr::audio_format::channels) getChannelCount() const;
    decltype(kfr::audio_format::type) getSampleType() const;
    AudioIO::AudioFormat::Container getContainerFormat() const;

    AudioIO::AudioFormat getFormat() const;

    // Sample rate auto mode
    bool isAutoSampleRate() const;
    void setAutoSampleRateValue(double value);

    // Bit depth (sample type) auto mode
    bool isAutoSampleType() const;
    void setAutoSampleTypeValue(kfr::audio_sample_type value);

    // Channel count auto mode
    bool isAutoChannelCount() const;
    void setAutoChannelCountValue(int value);

    // Set auto mode for all fields (MaxFromInput for combine, InheritFromInput for extract)
    void setAutoMode(AutoMode mode);
    void setAutoChannelMode(AutoMode mode); // Legacy alias

    void reset();

public slots:
    void showFormatHelp();

protected:
    void changeEvent(QEvent *event);

private slots:
    void onChannelsComboBoxChanged(int index);
    void onContainerFormatChanged(int index);

private:
    void setupSampleRateComboBox();
    void setupSampleTypeComboBox();
    void setupChannelsComboBox();
    void setupContainerFormatComboBox();
    void refreshComboBoxes();

    Ui::WAVFormatChooserWidget *ui;

    // Sample rate auto tracking
    int m_sampleRateAutoIndex = -1;   // Index of auto option in sample rate combobox
    double m_autoSampleRateValue = 0; // 0 means "inherit from input"

    // Sample type auto tracking
    int m_sampleTypeAutoIndex = -1; // Index of auto option in sample type combobox
    kfr::audio_sample_type m_autoSampleTypeValue =
        kfr::audio_sample_type::unknown; // unknown means "inherit from input"

    // Channel count auto tracking
    int m_customChannelIndex = -1; // Index of "Custom..." option
    int m_autoChannelValue = 0;    // 0 means "inherit from input"

    AutoMode m_autoMode = AutoMode::MaxFromInput;
};

#endif // WAVFORMATCHOOSERWIDGET_H
