#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QActionGroup>
#include <QMessageBox>
#include <QSettings>
#include <QValidator>
#include <QtConcurrent/QtConcurrent>

#include "dialogs/CommonHtmlDialog.h"
#include "dialogs/WavCombineDialog.h"
#include "dialogs/WavExtractDialog.h"
#include "utils/Filesystem.h"
#include "utils/TranslationManager.h"
#include "utils/UpdateChecker.h"
#include "utils/Utils.h"
#include "widgets/WavFormatChooserWidget.h"
#include "worker/AudioIO.h"

namespace {
constexpr const char *kSettingsGroupAudio = "audio";
constexpr const char *kSettingsKeySrcQuality = "sampleRateConversionQuality";

inline bool isValidSrcQualityInt(int v)
{
    using Q = kfr::sample_rate_conversion_quality;
    return v == static_cast<int>(Q::draft) || v == static_cast<int>(Q::low) || v == static_cast<int>(Q::normal) ||
           v == static_cast<int>(Q::high) || v == static_cast<int>(Q::perfect);
}
} // namespace

QMenu *MainWindow::createHelpMenu()
{
    auto helpMenu = new QMenu(this);
    auto aboutAction = helpMenu->addAction(tr("About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    helpMenu->addSeparator();
    helpMenu->addAction(UpdateChecker::createAutoCheckAction());
    auto checkUpdateAction = helpMenu->addAction(tr("Check update now"));
    connect(checkUpdateAction, &QAction::triggered, this, [this]() { UpdateChecker::checkManually(m_updateChecker); });

    return helpMenu;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFixedSize(sizeHint());
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    m_updateChecker = new UpdateChecker::GithubReleaseChecker("shine5402", "KiraWAVTar");
    UpdateChecker::triggerScheduledCheck(m_updateChecker);

    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::reset);
    connect(ui->modeButtonGroup, &QButtonGroup::idClicked, this, &MainWindow::updateStackWidgetIndex);
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::run);

    auto helpMenu = createHelpMenu();
    ui->helpButton->setMenu(helpMenu);

    connect(ui->combineDirPathWidget, &DirNameEditWithBrowse::browseTriggered, this, &MainWindow::fillResultPath);
    connect(ui->combineDirPathWidget, &DirNameEditWithBrowse::dropTriggered, this, &MainWindow::fillResultPath);
    connect(ui->extractSrcPathWidget, &FileNameEditWithBrowse::browseTriggered, this, &MainWindow::fillResultPath);
    connect(ui->extractSrcPathWidget, &FileNameEditWithBrowse::dropTriggered, this, &MainWindow::fillResultPath);

    // Set extract format widget to use "Inherit from input" mode
    ui->extractFormatCustomChooser->setAutoChannelMode(WAVFormatChooserWidget::AutoChannelMode::InheritFromInput);

    // i18n menu
    ui->langButton->setMenu(TranslationManager::getManager()->getI18nMenu());

    // Sample rate conversion quality menu
    {
        auto menu = new QMenu(this);
        auto group = new QActionGroup(this);
        group->setExclusive(true);

        auto qualityToText = [this](kfr::sample_rate_conversion_quality quality) -> QString {
            switch (quality) {
            case kfr::sample_rate_conversion_quality::draft:
                return tr("Draft");
            case kfr::sample_rate_conversion_quality::low:
                return tr("Low");
            case kfr::sample_rate_conversion_quality::normal:
                return tr("Normal");
            case kfr::sample_rate_conversion_quality::high:
                return tr("High");
            case kfr::sample_rate_conversion_quality::perfect:
                return tr("Perfect");
            }
            return tr("Normal");
        };

        auto addQualityAction = [&](const QString &text, kfr::sample_rate_conversion_quality quality) {
            auto action = menu->addAction(text);
            action->setCheckable(true);
            action->setData(static_cast<int>(quality));
            group->addAction(action);
            return action;
        };

        addQualityAction(tr("Draft"), kfr::sample_rate_conversion_quality::draft);
        addQualityAction(tr("Low"), kfr::sample_rate_conversion_quality::low);
        addQualityAction(tr("Normal"), kfr::sample_rate_conversion_quality::normal);
        addQualityAction(tr("High"), kfr::sample_rate_conversion_quality::high);
        addQualityAction(tr("Perfect"), kfr::sample_rate_conversion_quality::perfect);

        // Load persisted setting (default: Normal)
        {
            QSettings settings;
            settings.beginGroup(kSettingsGroupAudio);
            const int stored =
                settings.value(kSettingsKeySrcQuality, static_cast<int>(kfr::sample_rate_conversion_quality::normal))
                    .toInt();
            settings.endGroup();
            const int effective =
                isValidSrcQualityInt(stored) ? stored : static_cast<int>(kfr::sample_rate_conversion_quality::normal);
            utils::setSampleRateConversionQuality(static_cast<kfr::sample_rate_conversion_quality>(effective));
        }

        connect(group, &QActionGroup::triggered, this, [this](QAction *action) {
            bool ok = false;
            const int qualityInt = action->data().toInt(&ok);
            if (!ok)
                return;
            utils::setSampleRateConversionQuality(static_cast<kfr::sample_rate_conversion_quality>(qualityInt));

            QSettings settings;
            settings.beginGroup(kSettingsGroupAudio);
            settings.setValue(kSettingsKeySrcQuality, qualityInt);
            settings.endGroup();

            syncSrcQualityMenu();
        });

        ui->srcQualityButton->setMenu(menu);
        syncSrcQualityMenu();

        // Ensure button text shows current mode immediately
        ui->srcQualityButton->setText(
            tr("SRC Quality: %1").arg(qualityToText(utils::getSampleRateConversionQuality())));
    }
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

    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);

    QSettings settings;
    settings.beginGroup(kSettingsGroupAudio);
    settings.setValue(kSettingsKeySrcQuality, static_cast<int>(kfr::sample_rate_conversion_quality::normal));
    settings.endGroup();

    syncSrcQualityMenu();
}

void MainWindow::syncSrcQualityMenu()
{
    if (!ui->srcQualityButton || !ui->srcQualityButton->menu())
        return;

    auto qualityToText = [this](kfr::sample_rate_conversion_quality quality) -> QString {
        switch (quality) {
        case kfr::sample_rate_conversion_quality::draft:
            return tr("Draft");
        case kfr::sample_rate_conversion_quality::low:
            return tr("Low");
        case kfr::sample_rate_conversion_quality::normal:
            return tr("Normal");
        case kfr::sample_rate_conversion_quality::high:
            return tr("High");
        case kfr::sample_rate_conversion_quality::perfect:
            return tr("Perfect");
        }
        return tr("Normal");
    };

    const auto currentQuality = utils::getSampleRateConversionQuality();
    const int current = static_cast<int>(currentQuality);
    for (auto *action : ui->srcQualityButton->menu()->actions()) {
        bool ok = false;
        const int actionValue = action->data().toInt(&ok);
        if (ok) {
            action->setText(qualityToText(static_cast<kfr::sample_rate_conversion_quality>(actionValue)));
            action->setChecked(actionValue == current);
        }
    }

    ui->srcQualityButton->setText(tr("SRC Quality: %1").arg(qualityToText(currentQuality)));
}

void MainWindow::updateStackWidgetIndex()
{
    if (ui->combineWAVRadioButton->isChecked())
        ui->optionStackedWidget->setCurrentIndex(0); // Combine
    else
        ui->optionStackedWidget->setCurrentIndex(1); // Extract

    reset();
}

void MainWindow::run()
{
    if (ui->combineWAVRadioButton->isChecked()) {
        auto rootDirName = ui->combineDirPathWidget->dirName();
        auto recursive = ui->subdirCheckBox->isChecked();
        auto targetFormat = ui->combineWAVFormatWidget->getFormat();
        auto saveFileName = ui->combineResultPathWidget->fileName();

        if (rootDirName.isEmpty() || saveFileName.isEmpty()) {
            QMessageBox::critical(this, {}, tr("Needed paths are empty. Please check your input and try again."));
            return;
        }

        // Handle auto channel count: scan files to find max channel count
        if (ui->combineWAVFormatWidget->isAutoChannelCount()) {
            auto wavFileNames = getAbsoluteWAVFileNamesUnder(rootDirName, recursive);
            if (wavFileNames.isEmpty()) {
                QMessageBox::critical(this, {}, tr("No WAV files found in the specified folder."));
                return;
            }

            int maxChannels = 1;
            for (const auto &fileName : std::as_const(wavFileNames)) {
                try {
                    auto format = AudioIO::readWavFormat(fileName);
                    maxChannels = std::max(maxChannels, static_cast<int>(format.kfr_format.channels));
                } catch (...) {
                    // Ignore files that can't be read
                }
            }
            targetFormat.kfr_format.channels = maxChannels;
        }

        auto dialog = new WavCombineDialog(rootDirName, recursive, targetFormat, saveFileName, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
    } else {
        AudioIO::WavAudioFormat invalidFormat;
        invalidFormat.kfr_format.type = kfr::audio_sample_type::unknown;

        auto targetFormat =
            ui->extractFormatSrcRadioButton->isChecked() ? invalidFormat : ui->extractFormatCustomChooser->getFormat();
        auto srcWAVFileName = ui->extractSrcPathWidget->fileName();
        auto dstDirName = ui->extractResultPathWidget->dirName();
        auto extractResultSelection = ui->extractSelectionCheckBox->isChecked();
        auto removeDCOffset = ui->removeDCCheckBox->isChecked();
        if (srcWAVFileName.isEmpty() || dstDirName.isEmpty()) {
            QMessageBox::critical(this, {}, tr("Needed paths are empty. Please check your input and try again."));
            return;
        }
        auto dialog = new WavExtractDialog(srcWAVFileName, dstDirName, targetFormat, extractResultSelection,
                                           removeDCOffset, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
    }
}

void MainWindow::fillResultPath()
{
    if (ui->combineWAVRadioButton->isChecked()) {
        auto dirPath = ui->combineDirPathWidget->dirName();
        // Remove trailing separators before appending .wav
        while (!dirPath.isEmpty() && (dirPath.endsWith('/') || dirPath.endsWith('\\'))) {
            dirPath.chop(1);
        }
        ui->combineResultPathWidget->setFileName(dirPath + ".wav");
    } else {
        auto fileInfo = QFileInfo{ui->extractSrcPathWidget->fileName()};
        auto dir = fileInfo.absoluteDir();
        auto baseFileName = fileInfo.completeBaseName();
        ui->extractResultPathWidget->setDirName(dir.absoluteFilePath(baseFileName + "_result"));
    }
}

void MainWindow::about()
{
    QString versionStr =
        tr("<p>Version %1, <i>build on %2 %3<i></p>")
            .arg(qApp->applicationVersion().isEmpty() ? "(unknown)" : qApp->applicationVersion(), __DATE__, __TIME__);

    auto dialog = new CommonHtmlDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("About"));
    dialog->setHTML(tr(
                        R"(<p style="text-align: left;"><img src=":/icon/appIcon" width="64"/></p>
<h2>KiraWAVTar</h2>
<p>Copyright 2021-present <a href="https://shine5402.top/about-me">shine_5402</a></p>
%1
<h3>About</h3>
<p>A fast and easy-to-use WAV combine/extract tool.</p>
<h3>License</h3>
<p> This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.<br>
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.<br>
You should have received a copy of the GNU General Public License
along with this program.  If not, see <a href="https://www.gnu.org/licenses/">https://www.gnu.org/licenses/</a>.<br>
 In addition, as a special exception, the copyright holders give
 permission to link the code of portions of this program with the
 OpenSSL library under certain conditions as described in each
 individual source file, and distribute linked combinations
 including the two.<br>
 You must obey the GNU General Public License in all respects
 for all of the code used other than OpenSSL.  If you modify
 file(s) with this exception, you may extend this exception to your
 version of the file(s), but you are not obligated to do so.  If you
 do not wish to do so, delete this exception statement from your
 version.  If you delete this exception statement from all source
 files in the program, then also delete it here.</p>

<h3>Acknowledgements</h3>
<h4>Third-party libraries</h4>
<ul>
<li>Qt %2, The Qt Company Ltd, under LGPL v3.</li>
<li><a href="https://www.kfrlib.com/">KFR - Fast, modern C++ DSP framework</a>, under GNU GPL v2+</li>
</ul>
<h4>Translators</h4>
<p>Japanese translation was provided by Leka in previous versions. Their work continues to be partly used in the current version.
Thanks for their contribution!</p>
)")
                        .arg(versionStr)
                        .arg(QT_VERSION_STR));
    dialog->setStandardButtons(QDialogButtonBox::Ok);
    dialog->exec();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        ui->helpButton->menu()->deleteLater();
        ui->helpButton->setMenu(createHelpMenu());

        // Re-translate and re-sync text of SRC quality menu/button
        syncSrcQualityMenu();
    }
}
