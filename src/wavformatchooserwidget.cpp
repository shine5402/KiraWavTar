#include "wavformatchooserwidget.h"
#include "ui_wavformatchooserwidget.h"
#include <kfr/all.hpp>
#include "kfr_adapt.h"
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
        msgBox.setText("选择W64格式之前请三思。");
        msgBox.setInformativeText("请注意，W64格式与标准的WAV文件并不兼容。\n"
"其质量与标准WAV文件并无不同，但是其64位的文件头可以解决标准WAV文件中32位文件头带来的大小限制（约2GB/4GB，取决于软件实现）。"
"也因此该格式需要与标准WAV文件不同的代码进行读取和保存，还请注意确认您使用的音频软件是否支持该格式。\n"
"您也有可能在软件中看到使用“Sony Wave 64”或者“Sonic Foundry Wave 64”来指代该格式。以及，该格式和RF64格式不同，也留意。\n"
"以及该格式的推荐扩展名为“.w64”，如有必要请注意更改。");
        msgBox.exec();
    }
}
