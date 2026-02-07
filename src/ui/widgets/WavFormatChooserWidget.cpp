#include "WavFormatChooserWidget.h"
#include "ui_wavformatchooserwidget.h"

#include <kfr/all.hpp>

#include "ui/dialogs/CommonHtmlDialog.h"
#include "utils/KfrHelper.h"

WAVFormatChooserWidget::WAVFormatChooserWidget(QWidget *parent) : QWidget(parent), ui(new Ui::WAVFormatChooserWidget)
{
    ui->setupUi(this);

    // Insert separator after frequent sample rates (after index 2: 44100, 48000, 96000)
    ui->sampleRateComboBox->insertSeparator(3);

    for (const auto &[type, name] : kfr::audio_sample_type_entries) {
        ui->sampleTypeComboBox->addItem(name.data());
    }

    // Set up container format combobox
    // First section: RIFF (default), RF64
    ui->containerFormatComboBox->addItem(tr("RIFF (Standard WAV)"));
    ui->containerFormatComboBox->addItem(tr("RF64 (RIFF 64-bit)"));
    // Separator
    ui->containerFormatComboBox->insertSeparator(2);
    // Second section: W64
    ui->containerFormatComboBox->addItem(tr("W64 (Sony Wave64)"));

    reset();
}

WAVFormatChooserWidget::~WAVFormatChooserWidget()
{
    delete ui;
}

decltype(kfr::audio_format::samplerate) WAVFormatChooserWidget::getSampleRate() const
{
    return ui->sampleRateComboBox->currentText().toDouble();
}

decltype(kfr::audio_format::channels) WAVFormatChooserWidget::getChannelCount() const
{
    return ui->channelsSpinBox->value();
}

decltype(kfr::audio_format::type) WAVFormatChooserWidget::getSampleType() const
{
    return kfr::audio_sample_type_entries[ui->sampleTypeComboBox->currentIndex()].first;
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
    ui->sampleRateComboBox->setCurrentIndex(1); // 48000
    ui->channelsSpinBox->setValue(1);
    ui->sampleTypeComboBox->setCurrentIndex(4);
    ui->containerFormatComboBox->setCurrentIndex(0); // RIFF
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
