#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wavcombine.h"
#include "wavextract.h"
#include <QMessageBox>
#include <QValidator>
#include <kira/lib_helper/kfr_helper.h>
#include "wavcombinedialog.h"
#include "wavextractdialog.h"
#include <QPalette>
#include <kira/i18n/translationmanager.h>
#include <kira/darkmode.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFixedHeight(sizeHint().height());
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint |
                   Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::reset);
    connect(ui->modeButtonGroup, &QButtonGroup::idClicked, this, &MainWindow::updateStackWidgetIndex);
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::run);
    connect(ui->aboutButton, &QPushButton::clicked, this, &MainWindow::about);

    connect(ui->combineDirPathWidget, &DirNameEditWithBrowse::browseTriggered, this, &MainWindow::fillResultPath);
    connect(ui->combineDirPathWidget, &DirNameEditWithBrowse::dropTriggered, this, &MainWindow::fillResultPath);
    connect(ui->extractSrcPathWidget, &FileNameEditWithBrowse::browseTriggered, this, &MainWindow::fillResultPath);
    connect(ui->extractSrcPathWidget, &FileNameEditWithBrowse::dropTriggered, this, &MainWindow::fillResultPath);

    //Make line separator looks nicer
    QPalette linePalette;
    linePalette.setColor(QPalette::WindowText, linePalette.color(QPalette::Dark));
    ui->leftBottomButtonLine->setPalette(linePalette);

    //i18n menu
    ui->langButton->setMenu(TranslationManager::getManager()->getI18nMenu());

    //ui theme menu
    ui->uiThemeButton->setMenu(DarkMode::getDarkModeSettingMenu());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reset()
{
    ui->combineDirPathWidget->setDirName({});
    ui->subdirCheckBox->setChecked(true);
    ui->combineResultPathWidget->setFileName({});
    ui->combineWAVFormatWidget->reset();

    ui->extractSrcPathWidget->setFileName({});
    ui->extractResultPathWidget->setDirName({});
    ui->extractFormatSrcRadioButton->setChecked(true);
    ui->extractFormatCustomChooser->reset();
    ui->extractSelectionCheckBox->setChecked(false);
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

        if (rootDirName.isEmpty() || saveFileName.isEmpty()){
            QMessageBox::critical(this, {}, tr("Needed paths are empty. Please check your input and try again."));
            return;
        }

        auto dialog = new WAVCombineDialog(rootDirName, recursive, targetFormat, saveFileName, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
    }
    else
    {
        constexpr auto invalidFormat = kfr::audio_format{0, kfr::audio_sample_type::unknown, 0, false};
        auto targetFormat = ui->extractFormatSrcRadioButton->isChecked() ? invalidFormat : ui->extractFormatCustomChooser->getFormat();
        auto srcWAVFileName = ui->extractSrcPathWidget->fileName();
        auto dstDirName = ui->extractResultPathWidget->dirName();
        auto extractResultSelection = ui->extractSelectionCheckBox->isChecked();
        auto removeDCOffset = ui->removeDCCheckBox->isChecked();
        if (srcWAVFileName.isEmpty() || dstDirName.isEmpty()){
            QMessageBox::critical(this, {}, tr("Needed paths are empty. Please check your input and try again."));
            return;
        }
        auto dialog = new WAVExtractDialog(srcWAVFileName, dstDirName, targetFormat, extractResultSelection, removeDCOffset, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
    }

}

void MainWindow::fillResultPath()
{
    if (ui->combineWAVRadioButton->isChecked())
        //TODO: remove last (if exists) "/" and "\" on dirName, as on specific platform, they will have this on the end.
        ui->combineResultPathWidget->setFileName(ui->combineDirPathWidget->dirName() + ".wav");
    else
    {
        auto fileInfo = QFileInfo{ui->extractSrcPathWidget->fileName()};
        auto dir = fileInfo.absoluteDir();
        auto baseFileName = fileInfo.completeBaseName();
        ui->extractResultPathWidget->setDirName(dir.absoluteFilePath(baseFileName + "_result"));
    }
}

void MainWindow::about()
{
    auto versionStr = qApp->applicationVersion();
    QMessageBox msgBox;
    msgBox.setIconPixmap(QPixmap(":/icon/appIcon").scaled(64,64,Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setWindowTitle(tr("About"));
    msgBox.setText(tr(
                       R"(<h2>KiraWAVTar</h2>
<p>Copyright 2021-2022 <a href="https://shine5402.top/about-me">shine_5402</a></p>
<p>Version %1</p>
<h3>About</h3>
<p>A fast and easy-to-use WAV combine/extract tool.</p>
<h3>License</h3>
<p> This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.</p>
<p>This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.</p>
<p>You should have received a copy of the GNU General Public License
along with this program.  If not, see <a href="https://www.gnu.org/licenses/">https://www.gnu.org/licenses/</a>.</p>

<h3>3rd party libraries used by this project</h3>
<ul>
<li>Qt %2, The Qt Company Ltd, under LGPL v3.</li>
<li><a href="https://www.kfrlib.com/">KFR - Fast, modern C++ DSP framework</a>, under GNU GPL v2+</li>
<li><a href="https://github.com/shine5402/KiraCommonUtils">KiraCommmonUtils</a>, shine_5402, mainly under the Apache License, Version 2.0</li>
<li><a href="https://github.com/mapbox/eternal">eternal</a>, mapbox, under ISC License</li>
</ul>
)").arg(versionStr).arg(QT_VERSION_STR));
    msgBox.exec();
}

void MainWindow::changeEvent(QEvent* event){
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
}
