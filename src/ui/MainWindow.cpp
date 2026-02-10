#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QActionGroup>
#include <QDesktopServices>
#include <QMessageBox>
#include <QSettings>
#include <QValidator>
#include <QtConcurrent/QtConcurrent>

#include "dialogs/CommonHtmlDialog.h"
#include "dialogs/WavCombineDialog.h"
#include "dialogs/WavExtractDialog.h"
#include "utils/Filesystem.h"
#include "utils/KfrHelper.h"
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
    auto homepageAction = helpMenu->addAction(tr("Project homepage"));
    connect(homepageAction, &QAction::triggered, this,
            []() { QDesktopServices::openUrl(QUrl("https://github.com/shine5402/KiraWAVTar")); });
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
    connect(ui->combineWAVFormatWidget, &WAVFormatChooserWidget::containerFormatChanged, this,
            &MainWindow::updateCombineResultExtension);

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
    ui->combineGapSpinBox->setValue(0);
    ui->combineMultiVolumeCheckBox->setChecked(false);
    ui->volumeByCountRadioButton->setChecked(true);
    ui->volumeCountSpinBox->setValue(100);
    ui->volumeDurationSpinBox->setValue(300);

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

        // Handle auto modes: scan files to find max values
        bool needScan = ui->combineWAVFormatWidget->isAutoSampleRate() ||
                        ui->combineWAVFormatWidget->isAutoSampleType() ||
                        ui->combineWAVFormatWidget->isAutoChannelCount();

        if (needScan) {
            auto wavFileNames = getAbsoluteAudioFileNamesUnder(rootDirName, recursive);
            if (wavFileNames.isEmpty()) {
                QMessageBox::critical(this, {}, tr("No WAV files found in the specified folder."));
                return;
            }

            double maxSampleRate = 0;
            int maxSampleTypePrecision = 0;
            kfr::audio_sample_type maxSampleType = kfr::audio_sample_type::i16;
            int maxChannels = 1;

            for (const auto &fileName : std::as_const(wavFileNames)) {
                try {
                    auto format = AudioIO::readAudioFormat(fileName);

                    // Track max sample rate
                    maxSampleRate = std::max(maxSampleRate, format.kfr_format.samplerate);

                    // Track max sample type (by precision)
                    int precision = kfr::audio_sample_type_to_precision(format.kfr_format.type);
                    if (precision > maxSampleTypePrecision) {
                        maxSampleTypePrecision = precision;
                        maxSampleType = format.kfr_format.type;
                    }

                    // Track max channels
                    maxChannels = std::max(maxChannels, static_cast<int>(format.kfr_format.channels));
                } catch (...) {
                    // Ignore files that can't be read
                }
            }

            // Apply auto values
            if (ui->combineWAVFormatWidget->isAutoSampleRate() && maxSampleRate > 0) {
                targetFormat.kfr_format.samplerate = maxSampleRate;
            }
            if (ui->combineWAVFormatWidget->isAutoSampleType() && maxSampleTypePrecision > 0) {
                targetFormat.kfr_format.type = maxSampleType;
                // For FLAC, ensure auto sample type is integer (map float→int by byte width)
                if (targetFormat.isFlac()) {
                    targetFormat.kfr_format.type = kfr::to_flac_compatible_sample_type(targetFormat.kfr_format.type);
                }
            }
            if (ui->combineWAVFormatWidget->isAutoChannelCount()) {
                targetFormat.kfr_format.channels = maxChannels;
            }
        }

        auto gapMs = ui->combineGapSpinBox->value();

        utils::VolumeConfig volumeConfig;
        if (ui->combineMultiVolumeCheckBox->isChecked()) {
            if (ui->volumeByCountRadioButton->isChecked()) {
                volumeConfig.mode = utils::VolumeSplitMode::ByCount;
                volumeConfig.maxEntriesPerVolume = ui->volumeCountSpinBox->value();
            } else {
                volumeConfig.mode = utils::VolumeSplitMode::ByDuration;
                volumeConfig.maxDurationSeconds = ui->volumeDurationSpinBox->value();
            }
        }

        auto dialog =
            new WavCombineDialog(rootDirName, recursive, targetFormat, saveFileName, gapMs, volumeConfig, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
    } else {
        std::optional<AudioIO::AudioFormat> targetFormat;
        if (!ui->extractFormatSrcRadioButton->isChecked())
            targetFormat = ui->extractFormatCustomChooser->getFormat();
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
        // Remove trailing separators before appending extension
        while (!dirPath.isEmpty() && (dirPath.endsWith('/') || dirPath.endsWith('\\'))) {
            dirPath.chop(1);
        }
        // Choose extension based on container format
        auto container = ui->combineWAVFormatWidget->getContainerFormat();
        QString ext = (container == AudioIO::AudioFormat::Container::FLAC) ? ".flac" : ".wav";
        ui->combineResultPathWidget->setFileName(dirPath + ext);
    } else {
        auto fileInfo = QFileInfo{ui->extractSrcPathWidget->fileName()};
        auto dir = fileInfo.absoluteDir();
        auto baseFileName = fileInfo.completeBaseName();
        ui->extractResultPathWidget->setDirName(dir.absoluteFilePath(baseFileName + "_result"));
    }
}

void MainWindow::updateCombineResultExtension()
{
    auto currentPath = ui->combineResultPathWidget->fileName();
    if (currentPath.isEmpty())
        return;
    auto fileInfo = QFileInfo(currentPath);
    auto container = ui->combineWAVFormatWidget->getContainerFormat();
    QString newExt = (container == AudioIO::AudioFormat::Container::FLAC) ? "flac" : "wav";
    if (fileInfo.suffix().compare(newExt, Qt::CaseInsensitive) != 0) {
        auto newPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + "." + newExt;
        ui->combineResultPathWidget->setFileName(newPath);
    }
}

void MainWindow::about()
{
    QString versionStr =
        tr("<p>Version %1, <i>build on %2 %3</i></p>")
            .arg(qApp->applicationVersion().isEmpty() ? "(unknown)" : qApp->applicationVersion(), __DATE__, __TIME__);

    auto dialog = new CommonHtmlDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("About"));
    dialog->setHTML(tr(
                        R"(<p style="text-align: left;"><img src=":/icon/appIcon" width="64"/></p>
<h2>KiraWAVTar</h2>
<p>Copyright 2021-present shine_5402</p>
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
</p>

<h3>Acknowledgements</h3>
<h4>Third-party libraries</h4>
<ul>
<li>Qt %2, The Qt Company Ltd, under LGPL v3.</li>
<li><a href="https://www.kfrlib.com/">KFR - Fast, modern C++ DSP framework</a>, under GNU GPL v2+</li>
<li>dr_wav - Public domain single-header WAV reader/writer library by David Reid.</li>
<li>libFLAC - reference implementation of the Free Lossless Audio Codec, under BSD-3-Clause.</li>
</ul>
<h4>Translators</h4>
<p>This program includes translations with contributions from the community. See our <a href="https://github.com/shine5402/KiraWAVTar?tab=readme-ov-file#translations">homepage</a> for acknowledgements.</p>
)")
                        .arg(versionStr)
                        .arg(QT_VERSION_STR));
    dialog->setStandardButtons(QDialogButtonBox::Ok);
    dialog->setOpenExternalLinks(true);
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
