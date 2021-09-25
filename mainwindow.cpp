#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wavcombine.h"
#include "wavextract.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    //if (ui->combineWAVRadioButton->isChecked())
        //WAVCombine::doWork(ui->combineDirPathWidget->dirName(), ui->subdirCheckBox->isChecked(), ui->combineResultPathWidget->fileName());
    //else
        //WAVExtact::doWork(ui->extractSrcPathWidget->fileName(), ui->extractResultPathWidget->dirName());

    //QMessageBox::information(this, {}, tr("操作已经完成。"));
}

