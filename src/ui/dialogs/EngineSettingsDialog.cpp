#include "EngineSettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <kfr/all.hpp>

#include "utils/Utils.h"

namespace {
constexpr const char *kSettingsGroupAudio = "audio";
constexpr const char *kSettingsKeySrcQuality = "sampleRateConversionQuality";
constexpr const char *kSettingsKeyMaxLiveTokens = "maxLiveTokens";

struct SrcQualityEntry
{
    kfr::sample_rate_conversion_quality quality;
    const char *label;
};

constexpr SrcQualityEntry kSrcQualities[] = {
    {kfr::sample_rate_conversion_quality::draft, QT_TRANSLATE_NOOP("EngineSettingsDialog", "Draft")},
    {kfr::sample_rate_conversion_quality::low, QT_TRANSLATE_NOOP("EngineSettingsDialog", "Low")},
    {kfr::sample_rate_conversion_quality::normal, QT_TRANSLATE_NOOP("EngineSettingsDialog", "Normal")},
    {kfr::sample_rate_conversion_quality::high, QT_TRANSLATE_NOOP("EngineSettingsDialog", "High")},
    {kfr::sample_rate_conversion_quality::perfect, QT_TRANSLATE_NOOP("EngineSettingsDialog", "Perfect")},
};
} // namespace

EngineSettingsDialog::EngineSettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Engine Settings"));

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *mainLayout = new QVBoxLayout(this);

    // --- SRC Quality ---
    auto *srcGroup = new QGroupBox(tr("Sample Rate Conversion Quality"), this);
    auto *srcLayout = new QFormLayout(srcGroup);
    m_srcQualityCombo = new QComboBox(this);
    for (const auto &entry : kSrcQualities) {
        m_srcQualityCombo->addItem(tr(entry.label), static_cast<int>(entry.quality));
    }
    // Select current value
    const int currentQuality = static_cast<int>(utils::getSampleRateConversionQuality());
    for (int i = 0; i < m_srcQualityCombo->count(); ++i) {
        if (m_srcQualityCombo->itemData(i).toInt() == currentQuality) {
            m_srcQualityCombo->setCurrentIndex(i);
            break;
        }
    }
    srcLayout->addRow(tr("Quality:"), m_srcQualityCombo);
    mainLayout->addWidget(srcGroup);

    // --- Pipeline Concurrency ---
    auto *concGroup = new QGroupBox(tr("Pipeline Concurrency"), this);
    auto *concLayout = new QVBoxLayout(concGroup);

    auto *descLabel = new QLabel(tr("Controls how many audio chunks are processed simultaneously during combine.\n"
                                    "Higher values may improve speed but use more memory."),
                                 this);
    descLabel->setWordWrap(true);
    concLayout->addWidget(descLabel);

    m_autoCheckBox = new QCheckBox(tr("Auto (recommended)"), this);

    auto *sliderLayout = new QHBoxLayout;
    m_concurrencySlider = new QSlider(Qt::Horizontal, this);
    m_concurrencySlider->setRange(2, QThread::idealThreadCount());
    m_concurrencySpinBox = new QSpinBox(this);
    m_concurrencySpinBox->setRange(2, QThread::idealThreadCount());
    sliderLayout->addWidget(m_concurrencySlider);
    sliderLayout->addWidget(m_concurrencySpinBox);

    // Sync slider and spinbox
    connect(m_concurrencySlider, &QSlider::valueChanged, m_concurrencySpinBox, &QSpinBox::setValue);
    connect(m_concurrencySpinBox, &QSpinBox::valueChanged, m_concurrencySlider, &QSlider::setValue);

    // Auto checkbox disables manual controls
    connect(m_autoCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_concurrencySlider->setEnabled(!checked);
        m_concurrencySpinBox->setEnabled(!checked);
    });

    // Load current value
    const int currentTokens = utils::getMaxLiveTokens();
    if (currentTokens <= 0) {
        m_autoCheckBox->setChecked(true);
        m_concurrencySpinBox->setValue(utils::effectiveMaxLiveTokens());
    } else {
        m_autoCheckBox->setChecked(false);
        m_concurrencySpinBox->setValue(currentTokens);
    }

    concLayout->addWidget(m_autoCheckBox);
    concLayout->addLayout(sliderLayout);
    mainLayout->addWidget(concGroup);
    mainLayout->addStretch(1); // push groups to top

    // --- Button box ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EngineSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setFixedSize(sizeHint());
}

void EngineSettingsDialog::accept()
{
    // Apply SRC quality
    const int qualityInt = m_srcQualityCombo->currentData().toInt();
    utils::setSampleRateConversionQuality(static_cast<kfr::sample_rate_conversion_quality>(qualityInt));

    // Apply pipeline concurrency
    const int tokens = m_autoCheckBox->isChecked() ? 0 : m_concurrencySpinBox->value();
    utils::setMaxLiveTokens(tokens);

    // Persist to QSettings
    QSettings settings;
    settings.beginGroup(kSettingsGroupAudio);
    settings.setValue(kSettingsKeySrcQuality, qualityInt);
    settings.setValue(kSettingsKeyMaxLiveTokens, tokens);
    settings.endGroup();

    QDialog::accept();
}

QSize EngineSettingsDialog::sizeHint() const
{
    return QSize(480, heightForWidth(480));
}