#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wavcombine.h"
#include "wavextract.h"
#include <QMessageBox>
#include <QValidator>
#include "kfr_adapt.h"
#include "wavcombinedialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFixedHeight(sizeHint().height());
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    auto sampleRateValidator = new QIntValidator(2, INT_MAX);
    ui->sampleRateComboBox->setValidator(sampleRateValidator);

    for(const auto& item : kfr::audio_sample_type_human_string){
        ui->sampleTypeComboBox->addItem(item.second.c_str());
    }

    ui->sampleTypeComboBox->setCurrentText(kfr::audio_sample_type_human_string.at(kfr::audio_sample_type::i16).c_str());

    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::reset);
    connect(ui->modeButtonGroup, &QButtonGroup::idClicked, this, &MainWindow::updateStackWidgetIndex);
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::run);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reset()
{
    ui->combineDirPathWidget->setDirName({});
    ui->subdirCheckBox->setChecked(false);
    ui->combineResultPathWidget->setFileName({});

    ui->extractSrcPathWidget->setFileName({});
    ui->extractResultPathWidget->setDirName({});
}

void MainWindow::updateStackWidgetIndex()
{
    if (ui->combineWAVRadioButton->isChecked())
        ui->optionStackedWidget->setCurrentIndex(0);//Combine
    else
        ui->optionStackedWidget->setCurrentIndex(1);//Extract

    reset();
}

void MainWindow::run()
{
    if (ui->combineWAVRadioButton->isChecked())
    {
        auto rootDirName = ui->combineDirPathWidget->dirName();
        auto recursive = ui->subdirCheckBox->isChecked();
        auto targetFormat = kfr::audio_format{static_cast<size_t>(ui->channelsSpinBox->value()),
                             (kfr::audio_sample_type_human_string.begin() + ui->sampleTypeComboBox->currentIndex())->first,
                            ui->sampleRateComboBox->currentText().toDouble(),
                            ui->useW64CheckBox->isChecked()};
        auto saveFileName = ui->combineResultPathWidget->fileName();
        auto dialog = new WAVCombineDialog(rootDirName, recursive, targetFormat, saveFileName, this);
        dialog->open();
    }
    else
        WAVExtact::doWork(ui->extractSrcPathWidget->fileName(), ui->extractResultPathWidget->dirName());

}

