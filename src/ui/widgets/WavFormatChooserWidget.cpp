#include "WavFormatChooserWidget.h"
#include "ui_wavformatchooserwidget.h"

#include <kfr/all.hpp>

#include "ui/dialogs/CommonHtmlDialog.h"
#include "utils/KfrHelper.h"

WAVFormatChooserWidget::WAVFormatChooserWidget(QWidget *parent) : QWidget(parent), ui(new Ui::WAVFormatChooserWidget)
{
    ui->setupUi(this);

    refreshComboBoxes();
    connect(ui->channelsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WAVFormatChooserWidget::onChannelsComboBoxChanged, Qt::UniqueConnection);

    reset();
}

WAVFormatChooserWidget::~WAVFormatChooserWidget()
{
    delete ui;
}

decltype(kfr::audio_format::samplerate) WAVFormatChooserWidget::getSampleRate() const
{
    int index = ui->sampleRateComboBox->currentIndex();
    if (index == m_sampleRateAutoIndex) {
        // Auto: return the auto-detected value
        return m_autoSampleRateValue;
    } else {
        // Preset: parse from text
        return ui->sampleRateComboBox->currentText().toDouble();
    }
}

decltype(kfr::audio_format::channels) WAVFormatChooserWidget::getChannelCount() const
{
    int index = ui->channelsComboBox->currentIndex();
    if (index == ChannelIndexAuto) {
        // Auto: return the auto-detected value
        return m_autoChannelValue;
    } else if (index == m_customChannelIndex) {
        // Custom: return from spinbox
        return ui->channelsSpinBox->value();
    } else {
        // Preset: extract the number before the space/parenthesis
        QString text = ui->channelsComboBox->currentText();
        return text.split(' ').first().toInt();
    }
}

decltype(kfr::audio_format::type) WAVFormatChooserWidget::getSampleType() const
{
    int index = ui->sampleTypeComboBox->currentIndex();
    if (index == m_sampleTypeAutoIndex) {
        // Auto: return the auto-detected value
        return m_autoSampleTypeValue;
    } else {
        // Preset: get from entries (accounting for auto option and separator)
        int presetIndex = index - 2; // Subtract auto option (0) and separator (1)
        if (presetIndex >= 0 && presetIndex < static_cast<int>(kfr::audio_sample_type_entries_for_ui.size())) {
            return kfr::audio_sample_type_entries_for_ui[presetIndex].first;
        }
        return kfr::audio_sample_type::f32; // Default fallback
    }
}

AudioIO::WavAudioFormat::Container WAVFormatChooserWidget::getWAVContainerFormat() const
{
    int index = ui->containerFormatComboBox->currentIndex();
    // Account for separator at index 2
    if (index == 0)
        return AudioIO::WavAudioFormat::Container::RIFF;
    else if (index == 1)
        return AudioIO::WavAudioFormat::Container::RF64;
    else if (index == 3) // After separator
        return AudioIO::WavAudioFormat::Container::W64;
    return AudioIO::WavAudioFormat::Container::RIFF; // Default fallback
}

AudioIO::WavAudioFormat WAVFormatChooserWidget::getFormat() const
{
    AudioIO::WavAudioFormat fmt;
    fmt.kfr_format.channels = getChannelCount();
    fmt.kfr_format.samplerate = getSampleRate();
    fmt.kfr_format.type = getSampleType();
    fmt.container = getWAVContainerFormat();
    fmt.length = 0;
    return fmt;
}

void WAVFormatChooserWidget::reset()
{
    ui->sampleRateComboBox->setCurrentIndex(m_sampleRateAutoIndex); // Auto
    ui->sampleTypeComboBox->setCurrentIndex(m_sampleTypeAutoIndex); // Auto
    ui->channelsComboBox->setCurrentIndex(ChannelIndexAuto);        // Auto
    ui->channelsSpinBox->setValue(2);
    ui->channelsSpinBox->setVisible(false);
    ui->containerFormatComboBox->setCurrentIndex(0); // RIFF
}

bool WAVFormatChooserWidget::isAutoSampleRate() const
{
    return ui->sampleRateComboBox->currentIndex() == m_sampleRateAutoIndex;
}

void WAVFormatChooserWidget::setAutoSampleRateValue(double value)
{
    m_autoSampleRateValue = value;
}

bool WAVFormatChooserWidget::isAutoSampleType() const
{
    return ui->sampleTypeComboBox->currentIndex() == m_sampleTypeAutoIndex;
}

void WAVFormatChooserWidget::setAutoSampleTypeValue(kfr::audio_sample_type value)
{
    m_autoSampleTypeValue = value;
}

bool WAVFormatChooserWidget::isAutoChannelCount() const
{
    return ui->channelsComboBox->currentIndex() == ChannelIndexAuto;
}

void WAVFormatChooserWidget::setAutoChannelCountValue(int value)
{
    m_autoChannelValue = value;
}

void WAVFormatChooserWidget::onChannelsComboBoxChanged(int index)
{
    // Show spinbox only when "Custom..." is selected
    ui->channelsSpinBox->setVisible(index == m_customChannelIndex);
}

void WAVFormatChooserWidget::setAutoMode(AutoMode mode)
{
    m_autoMode = mode;
    setupSampleRateComboBox();
    setupSampleTypeComboBox();
    setupChannelsComboBox();
}

void WAVFormatChooserWidget::setAutoChannelMode(AutoMode mode)
{
    // Legacy method - just call setAutoMode
    setAutoMode(mode);
}

void WAVFormatChooserWidget::setupSampleRateComboBox()
{
    int currentIndex = ui->sampleRateComboBox->currentIndex();
    ui->sampleRateComboBox->clear();

    // Auto/Inherit option (index 0)
    if (m_autoMode == AutoMode::MaxFromInput) {
        ui->sampleRateComboBox->addItem(tr("Auto (max from input)"));
    } else {
        ui->sampleRateComboBox->addItem(tr("Inherit from input"));
    }
    m_sampleRateAutoIndex = 0;

    // Separator
    ui->sampleRateComboBox->insertSeparator(1);

    // Common sample rates - frequent ones first
    ui->sampleRateComboBox->addItem(QStringLiteral("44100"));
    ui->sampleRateComboBox->addItem(QStringLiteral("48000"));
    ui->sampleRateComboBox->addItem(QStringLiteral("96000"));

    // Separator before less common rates
    ui->sampleRateComboBox->insertSeparator(5);

    // Less common sample rates
    ui->sampleRateComboBox->addItem(QStringLiteral("11025"));
    ui->sampleRateComboBox->addItem(QStringLiteral("22050"));
    ui->sampleRateComboBox->addItem(QStringLiteral("88200"));
    ui->sampleRateComboBox->addItem(QStringLiteral("192000"));
    ui->sampleRateComboBox->addItem(QStringLiteral("384000"));

    // Restore selection if valid
    if (currentIndex >= 0 && currentIndex < ui->sampleRateComboBox->count()) {
        ui->sampleRateComboBox->setCurrentIndex(currentIndex);
    } else {
        ui->sampleRateComboBox->setCurrentIndex(m_sampleRateAutoIndex);
    }
}

void WAVFormatChooserWidget::setupSampleTypeComboBox()
{
    int currentIndex = ui->sampleTypeComboBox->currentIndex();
    ui->sampleTypeComboBox->clear();

    // Auto/Inherit option (index 0)
    if (m_autoMode == AutoMode::MaxFromInput) {
        ui->sampleTypeComboBox->addItem(tr("Auto (max from input)"));
    } else {
        ui->sampleTypeComboBox->addItem(tr("Inherit from input"));
    }
    m_sampleTypeAutoIndex = 0;

    // Separator
    ui->sampleTypeComboBox->insertSeparator(1);

    // Sample type presets
    for (const auto &[type, name] : kfr::audio_sample_type_entries_for_ui) {
        ui->sampleTypeComboBox->addItem(name.data());
    }

    // Restore selection if valid
    if (currentIndex >= 0 && currentIndex < ui->sampleTypeComboBox->count()) {
        ui->sampleTypeComboBox->setCurrentIndex(currentIndex);
    } else {
        ui->sampleTypeComboBox->setCurrentIndex(m_sampleTypeAutoIndex);
    }
}

void WAVFormatChooserWidget::setupChannelsComboBox()
{
    int currentIndex = ui->channelsComboBox->currentIndex();
    ui->channelsComboBox->clear();

    // Auto/Inherit option (index 0)
    if (m_autoMode == AutoMode::MaxFromInput) {
        ui->channelsComboBox->addItem(tr("Auto (max from input)"));
    } else {
        ui->channelsComboBox->addItem(tr("Inherit from input"));
    }

    // Separator
    ui->channelsComboBox->insertSeparator(1);

    // Common presets
    ui->channelsComboBox->addItem(tr("1 (Mono)"));
    ui->channelsComboBox->addItem(tr("2 (Stereo)"));
    ui->channelsComboBox->addItem(tr("6 (5.1 Surround)"));
    ui->channelsComboBox->addItem(tr("8 (7.1 Surround)"));

    // Separator before custom
    ui->channelsComboBox->insertSeparator(6);

    // Custom option
    ui->channelsComboBox->addItem(tr("Custom..."));
    m_customChannelIndex = 7; // After separator

    if (currentIndex == 1 || currentIndex == 6) {
        currentIndex = ChannelIndexAuto;
    }
    if (currentIndex >= 0 && currentIndex < ui->channelsComboBox->count()) {
        ui->channelsComboBox->setCurrentIndex(currentIndex);
    } else {
        ui->channelsComboBox->setCurrentIndex(ChannelIndexAuto);
    }

    onChannelsComboBoxChanged(ui->channelsComboBox->currentIndex());
}

void WAVFormatChooserWidget::showFormatHelp()
{
    auto dialog = new CommonHtmlDialog(this);
    dialog->setWindowTitle(tr("WAV Container Format Information"));
    dialog->setMarkdown(tr("## WAV Container Formats\n\n"
                           "WAV files can use different container formats, each with different limitations and "
                           "compatibility:\n\n"
                           "### RIFF (Standard WAV)\n"
                           "- **Maximum file size:** 4 GB\n"
                           "- **Compatibility:** Universally supported by all audio software and hardware\n"
                           "- **Use when:** File size is less than 4 GB and maximum compatibility is needed\n\n"
                           "### RF64 (RIFF 64-bit)\n"
                           "- **Maximum file size:** No practical limit (supports files larger than 4 GB)\n"
                           "- **Compatibility:** Supported by many modern professional audio applications "
                           "(Pro Tools, Reaper, Audacity, Adobe Audition, etc.), but **not universally supported**\n"
                           "- **Standard:** EBU Tech 3306 (European Broadcasting Union standard)\n"
                           "- **Use when:** Files exceed 4 GB and your audio software supports RF64\n\n"
                           "### W64 (Sony Wave64)\n"
                           "- **Maximum file size:** No practical limit (supports files larger than 4 GB)\n"
                           "- **Compatibility:** Supported by several professional audio applications "
                           "(Sound Forge, Vegas, Reaper, etc.), but **less widely supported than RF64**\n"
                           "- **Origin:** Proprietary format developed by Sony\n"
                           "- **Use when:** Files exceed 4 GB and your specific software prefers or requires W64\n\n"
                           "---\n\n"
                           "**Important:** Before using 64-bit formats (RF64 or W64), verify that your audio editing "
                           "software, DAW, or playback tools support the specific format you choose. Not all players "
                           "and editors support these extended formats. If compatibility is uncertain, test with a "
                           "small sample file first."));
    dialog->setStandardButtons(QDialogButtonBox::Ok);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void WAVFormatChooserWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        refreshComboBoxes();
    }
}

void WAVFormatChooserWidget::refreshComboBoxes()
{
    // Set up sample rate combobox with auto option
    setupSampleRateComboBox();

    // Set up sample type combobox with auto option
    setupSampleTypeComboBox();

    // Set up channels combobox
    setupChannelsComboBox();

    // Set up container format combobox
    setupContainerFormatComboBox();
}

void WAVFormatChooserWidget::setupContainerFormatComboBox()
{
    int currentIndex = ui->containerFormatComboBox->currentIndex();
    ui->containerFormatComboBox->clear();

    // First section: RIFF (default), RF64
    ui->containerFormatComboBox->addItem(tr("RIFF (Standard WAV)"));
    ui->containerFormatComboBox->addItem(tr("RF64 (RIFF 64-bit)"));
    // Separator
    ui->containerFormatComboBox->insertSeparator(2);
    // Second section: W64
    ui->containerFormatComboBox->addItem(tr("W64 (Sony Wave64)"));

    if (currentIndex == 2) {
        currentIndex = 0;
    }
    if (currentIndex >= 0 && currentIndex < ui->containerFormatComboBox->count()) {
        ui->containerFormatComboBox->setCurrentIndex(currentIndex);
    } else {
        ui->containerFormatComboBox->setCurrentIndex(0);
    }
}