#include "WavFormatChooserWidget.h"
#include "ui_wavformatchooserwidget.h"
#include <kfr/all.hpp>
#include "KfrHelper.h"
#include <QMessageBox>

WAVFormatChooserWidget::WAVFormatChooserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WAVFormatChooserWidget)
{
    ui->setupUi(this);

    auto sampleRateValidator = new QIntValidator(2, INT_MAX);
    ui->sampleRateComboBox->setValidator(sampleRateValidator);

    for(const auto& [type, name] : kfr::audio_sample_type_entries){
        ui->sampleTypeComboBox->addItem(name.data());
    }

    reset();

    connect(ui->use64BitCheckBox, &QCheckBox::clicked, this, &WAVFormatChooserWidget::warnAbout64Bit);
}

WAVFormatChooserWidget::~WAVFormatChooserWidget()
{
    delete ui;
}

decltype (kfr::audio_format::samplerate) WAVFormatChooserWidget::getSampleRate() const
{
    return ui->sampleRateComboBox->currentText().toDouble();
}

decltype (kfr::audio_format::channels) WAVFormatChooserWidget::getChannelCount() const
{
    return ui->channelsSpinBox->value();
}

decltype(kfr::audio_format::type) WAVFormatChooserWidget::getSampleType() const
{
    return kfr::audio_sample_type_entries[ui->sampleTypeComboBox->currentIndex()].first;
}

decltype (kfr::audio_format::wav_format) WAVFormatChooserWidget::getWAVContainerFormat() const
{
    if (!ui->use64BitCheckBox->isChecked())
        return kfr::audio_format::riff;
    if (ui->wav64BitFormatComboBox->currentIndex() == 0)
        return kfr::audio_format::rf64;
    else
        return kfr::audio_format::w64;

    return kfr::audio_format::riff;
}


void WAVFormatChooserWidget::setSampleRate(decltype(kfr::audio_format::samplerate) value)
{
    ui->sampleRateComboBox->setCurrentText(QString("%1").arg(value));
}

void WAVFormatChooserWidget::setChannelCount(decltype (kfr::audio_format::channels) value){
    ui->channelsSpinBox->setValue(value);
}

void WAVFormatChooserWidget::setSampleType(decltype(kfr::audio_format::type) value)
{
    ui->sampleTypeComboBox->setCurrentText(kfr::audio_sample_type_to_string(value).data());
}

void WAVFormatChooserWidget::setWAVContainerFormat(decltype(kfr::audio_format::wav_format) value)
{
    switch (value){
        case kfr::audio_format::riff:
            ui->use64BitCheckBox->setChecked(false);
            ui->wav64BitFormatComboBox->setCurrentIndex(0);
            break;
        case kfr::audio_format::rf64:
            ui->use64BitCheckBox->setChecked(true);
            ui->wav64BitFormatComboBox->setCurrentIndex(0);
            break;
        case kfr::audio_format::w64:
            ui->use64BitCheckBox->setChecked(true);
            ui->wav64BitFormatComboBox->setCurrentIndex(1);
            break;
    }
}

kfr::audio_format WAVFormatChooserWidget::getFormat() const
{
    return {getChannelCount(), getSampleType(), getSampleRate(), getWAVContainerFormat()};
}

void WAVFormatChooserWidget::setFormat(kfr::audio_format value)
{
    setChannelCount(value.channels);
    setSampleType(value.type);
    setSampleRate(value.samplerate);
    setWAVContainerFormat(value.wav_format);
}

void WAVFormatChooserWidget::reset()
{
    setChannelCount(1);
    setSampleRate(44100);
    setSampleType(kfr::audio_sample_type::i16);
    setWAVContainerFormat(kfr::audio_format::riff);
}

void WAVFormatChooserWidget::warnAbout64Bit()
{
    if (ui->use64BitCheckBox->isChecked()){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(tr("Think twice before choose 64-bit format."));
        msgBox.setInformativeText(tr("Please notice that 64-bit wav format is not compatible with standard (a.k.a RIFF) WAV files. "
                                  "You may face errors when use them in softwares that not have good supporting for them.\n"
                                  "The quality has no difference between these formats, "
                                  "but 64-bit header in coresponding formats can solve size restriction in standard 32-bit WAV format "
                                  "(~2GB/4GB, based on implementation). \n"
                                  "RF64 and W64 all have their own pros and cons, so choose what meets your need."));
        msgBox.exec();
    }
}

void WAVFormatChooserWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
}
