#include "wavformatchooserwidget.h"
#include "ui_wavformatchooserwidget.h"
#include <kfr/all.hpp>
#include <kira/lib_helper/kfr_helper.h>
#include <QMessageBox>

WAVFormatChooserWidget::WAVFormatChooserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WAVFormatChooserWidget)
{
    ui->setupUi(this);

    auto sampleRateValidator = new QIntValidator(2, INT_MAX);
    ui->sampleRateComboBox->setValidator(sampleRateValidator);

    for(const auto& item : kfr::audio_sample_type_human_string){
        ui->sampleTypeComboBox->addItem(item.second.c_str());
    }

    reset();

    connect(ui->useW64CheckBox, &QCheckBox::clicked, this, &WAVFormatChooserWidget::warnAboutW64);
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
    return (kfr::audio_sample_type_human_string.begin() + ui->sampleTypeComboBox->currentIndex())->first;
}

decltype (kfr::audio_format::use_w64) WAVFormatChooserWidget::getUseW64() const
{
    return ui->useW64CheckBox->isChecked();
}

void WAVFormatChooserWidget::setSampleRate(decltype (kfr::audio_format::samplerate) value)
{
    ui->sampleRateComboBox->setCurrentText(QString("%1").arg(value));
}

void WAVFormatChooserWidget::setChannelCount(decltype (kfr::audio_format::channels) value){
    ui->channelsSpinBox->setValue(value);
}

void WAVFormatChooserWidget::setSampleType(decltype (kfr::audio_format::type) value)
{
    ui->sampleTypeComboBox->setCurrentText(kfr::audio_sample_type_human_string.at(value).c_str());
}

void WAVFormatChooserWidget::setUseW64(decltype (kfr::audio_format::use_w64) value)
{
    ui->useW64CheckBox->setChecked(value);
}

kfr::audio_format WAVFormatChooserWidget::getFormat() const
{
    return {getChannelCount(), getSampleType(), getSampleRate(), getUseW64()};
}

void WAVFormatChooserWidget::setFormat(kfr::audio_format value)
{
    setChannelCount(value.channels);
    setSampleType(value.type);
    setSampleRate(value.samplerate);
    setUseW64(value.use_w64);
}

void WAVFormatChooserWidget::reset()
{
    setChannelCount(1);
    setSampleRate(44100);
    setSampleType(kfr::audio_sample_type::i16);
    setUseW64(false);
}

void WAVFormatChooserWidget::warnAboutW64()
{
    if (ui->useW64CheckBox->isChecked()){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("Think twice before choose W64 format.");
        msgBox.setInformativeText("Please notice that W64 format is not compatible with standard WAV files.\n"
                                  "The quality has no difference between both formats, "
                                  "but its 64-bit header can solve size restriction in standard 32-bit WAV format "
                                  "(~2GB/4GB, based on implementation). \n"
                                  "You may also see softwares use \"Sony Wave 64\" or \"Sonic Foundry Wave 64\" "
                                  "to refer to this format. Also, this format is not same as RF64(use as default 64-bit WAV in Audition), "
                                  "please pay attention to it. \n"
                                  "And this format uses \".wav\" for recommended extension. "
                                  "Change it if you need.");
        msgBox.exec();
    }
}
