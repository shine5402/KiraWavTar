#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wavcombine.h"
#include "wavextract.h"
#include <QMessageBox>
#include <QValidator>
#include "kfr_adapt.h"
#include "wavcombinedialog.h"
#include "wavextractdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFixedHeight(sizeHint().height());
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

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
    ui->combineWAVFormatWidget->reset();

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
        auto targetFormat = ui->combineWAVFormatWidget->getFormat();
        auto saveFileName = ui->combineResultPathWidget->fileName();
        auto dialog = new WAVCombineDialog(rootDirName, recursive, targetFormat, saveFileName, this);
        dialog->open();
    }
    else
    {
        constexpr auto invalidFormat = kfr::audio_format{0, kfr::audio_sample_type::unknown, 0, false};
        auto targetFormat = ui->extractFormatSrcRadioButton->isChecked() ? invalidFormat : ui->extractFormatCustomChooser->getFormat();
        auto srcWAVFileName = ui->extractSrcPathWidget->fileName();
        auto dstDirName = ui->extractResultPathWidget->dirName();
        auto dialog = new WAVExtractDialog(srcWAVFileName, dstDirName, targetFormat, this);
        dialog->open();
    }

}

