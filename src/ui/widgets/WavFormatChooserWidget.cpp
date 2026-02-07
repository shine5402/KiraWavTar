#include "WavFormatChooserWidget.h"
#include "ui_wavformatchooserwidget.h"

#include <QMessageBox>
#include <kfr/all.hpp>

#include "utils/KfrHelper.h"

WAVFormatChooserWidget::WAVFormatChooserWidget(QWidget *parent) : QWidget(parent), ui(new Ui::WAVFormatChooserWidget)
{
    ui->setupUi(this);

    auto sampleRateValidator = new QIntValidator(2, INT_MAX);
    ui->sampleRateComboBox->setValidator(sampleRateValidator);

    for (const auto &[type, name] : kfr::audio_sample_type_entries) {
        ui->sampleTypeComboBox->addItem(name.data());
    }

    reset();

    connect(ui->use64BitCheckBox, &QCheckBox::clicked, this, &WAVFormatChooserWidget::warnAbout64Bit);
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
    if (!ui->use64BitCheckBox->isChecked())
        return AudioIO::WavAudioFormat::Container::RIFF;
    if (ui->wav64BitFormatComboBox->currentIndex() == 0)
        return AudioIO::WavAudioFormat::Container::RF64;
    return AudioIO::WavAudioFormat::Container::W64;
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
    ui->sampleRateComboBox->setCurrentText("48000");
    ui->channelsSpinBox->setValue(1);
    ui->sampleTypeComboBox->setCurrentIndex(4);

    ui->use64BitCheckBox->setChecked(false);
    ui->wav64BitFormatComboBox->setCurrentIndex(0);
}

void WAVFormatChooserWidget::warnAbout64Bit(bool checked)
{
    if (checked) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("You are using 64-bit WAV format. "
                                "This format is not supported by all players and editors. "
                                "Make sure your tools support it."));
    }
}
