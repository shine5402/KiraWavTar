#include <QTest>
#include <QComboBox>
#include <QSpinBox>
#include <QSignalSpy>
#include <kfr/all.hpp>

#include "ui/widgets/WavFormatChooserWidget.h"
#include "worker/AudioIO.h"

class TestWavFormatChooser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void widget_sampleRate_autoMode();
    void widget_sampleRate_manualMode();
    void widget_sampleRate_setAutoValue();

    void widget_sampleType_autoMode();
    void widget_sampleType_manualMode();
    void widget_sampleType_flacFiltersToInteger();

    void widget_channels_autoMode();
    void widget_channels_manualMode_preset();
    void widget_channels_customValue();

    void widget_containerFormat_riff();
    void widget_containerFormat_flac();
    void widget_containerFormat_rf64();
    void widget_containerFormat_w64();
    void widget_containerFormat_signalEmitted();

    void widget_autoMode_combineVsExtract();

    void widget_getFormat_complete();
    void widget_reset();

private:
    WAVFormatChooserWidget *m_widget = nullptr;
};

void TestWavFormatChooser::initTestCase()
{
    m_widget = new WAVFormatChooserWidget();
    m_widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_widget));
}

void TestWavFormatChooser::cleanupTestCase()
{
    delete m_widget;
}

void TestWavFormatChooser::widget_sampleRate_autoMode()
{
    m_widget->reset();
    QVERIFY(m_widget->isAutoSampleRate());
    QCOMPARE(m_widget->getSampleRate(), 0.0);
}

void TestWavFormatChooser::widget_sampleRate_manualMode()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("sampleRateComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(2);
    QTest::qWait(10);

    QVERIFY(!m_widget->isAutoSampleRate());
    QCOMPARE(m_widget->getSampleRate(), 44100.0);

    comboBox->setCurrentIndex(3);
    QTest::qWait(10);
    QCOMPARE(m_widget->getSampleRate(), 48000.0);

    comboBox->setCurrentIndex(4);
    QTest::qWait(10);
    QCOMPARE(m_widget->getSampleRate(), 96000.0);
}

void TestWavFormatChooser::widget_sampleRate_setAutoValue()
{
    m_widget->reset();
    m_widget->setAutoSampleRateValue(88200.0);

    QVERIFY(m_widget->isAutoSampleRate());
    QCOMPARE(m_widget->getSampleRate(), 88200.0);
}

void TestWavFormatChooser::widget_sampleType_autoMode()
{
    m_widget->reset();
    QVERIFY(m_widget->isAutoSampleType());
    QCOMPARE(m_widget->getSampleType(), kfr::audio_sample_type::unknown);
}

void TestWavFormatChooser::widget_sampleType_manualMode()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("sampleTypeComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(2);
    QTest::qWait(10);

    QVERIFY(!m_widget->isAutoSampleType());
    QCOMPARE(m_widget->getSampleType(), kfr::audio_sample_type::i16);

    comboBox->setCurrentIndex(3);
    QTest::qWait(10);
    QCOMPARE(m_widget->getSampleType(), kfr::audio_sample_type::i24);

    comboBox->setCurrentIndex(4);
    QTest::qWait(10);
    QCOMPARE(m_widget->getSampleType(), kfr::audio_sample_type::i32);
}

void TestWavFormatChooser::widget_sampleType_flacFiltersToInteger()
{
    m_widget->reset();

    auto *containerComboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(containerComboBox != nullptr);

    containerComboBox->setCurrentIndex(1);
    QTest::qWait(10);

    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::FLAC);

    auto *sampleTypeComboBox = m_widget->findChild<QComboBox *>("sampleTypeComboBox");
    QVERIFY(sampleTypeComboBox != nullptr);

    for (int i = 0; i < sampleTypeComboBox->count(); ++i) {
        QString text = sampleTypeComboBox->itemText(i);
        if (text.isEmpty() || i == m_widget->AutoIndex) continue;
        
        sampleTypeComboBox->setCurrentIndex(i);
        QTest::qWait(10);

        auto type = m_widget->getSampleType();
        QVERIFY2(type == kfr::audio_sample_type::i16 ||
                 type == kfr::audio_sample_type::i24 ||
                 type == kfr::audio_sample_type::i32,
                 QString("FLAC should only allow integer sample types, got: %1 at index %2")
                     .arg(static_cast<int>(type))
                     .arg(i)
                     .toUtf8());
    }
}

void TestWavFormatChooser::widget_channels_autoMode()
{
    m_widget->reset();
    QVERIFY(m_widget->isAutoChannelCount());
    QCOMPARE(m_widget->getChannelCount(), 0);
}

void TestWavFormatChooser::widget_channels_manualMode_preset()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("channelsComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(2);
    QTest::qWait(10);

    QVERIFY(!m_widget->isAutoChannelCount());
    QCOMPARE(m_widget->getChannelCount(), 1);

    comboBox->setCurrentIndex(3);
    QTest::qWait(10);
    QCOMPARE(m_widget->getChannelCount(), 2);

    comboBox->setCurrentIndex(4);
    QTest::qWait(10);
    QCOMPARE(m_widget->getChannelCount(), 6);

    comboBox->setCurrentIndex(5);
    QTest::qWait(10);
    QCOMPARE(m_widget->getChannelCount(), 8);
}

void TestWavFormatChooser::widget_channels_customValue()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("channelsComboBox");
    auto *spinBox = m_widget->findChild<QSpinBox *>("channelsSpinBox");
    QVERIFY(comboBox != nullptr);
    QVERIFY(spinBox != nullptr);

    comboBox->setCurrentIndex(7);
    QTest::qWait(10);

    QVERIFY(spinBox->isVisible());
    QVERIFY(!m_widget->isAutoChannelCount());

    spinBox->setValue(12);
    QTest::qWait(10);
    QCOMPARE(m_widget->getChannelCount(), 12);
}

void TestWavFormatChooser::widget_containerFormat_riff()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(0);
    QTest::qWait(10);

    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::RIFF);
}

void TestWavFormatChooser::widget_containerFormat_flac()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(1);
    QTest::qWait(10);

    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::FLAC);
}

void TestWavFormatChooser::widget_containerFormat_rf64()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(3);
    QTest::qWait(10);

    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::RF64);
}

void TestWavFormatChooser::widget_containerFormat_w64()
{
    m_widget->reset();

    auto *comboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(4);
    QTest::qWait(10);

    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::W64);
}

void TestWavFormatChooser::widget_containerFormat_signalEmitted()
{
    m_widget->reset();

    QSignalSpy spy(m_widget, SIGNAL(containerFormatChanged(AudioIO::AudioFormat::Container)));

    auto *comboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    QVERIFY(comboBox != nullptr);

    comboBox->setCurrentIndex(1);
    QTest::qWait(10);

    QCOMPARE(spy.count(), 1);

    comboBox->setCurrentIndex(3);
    QTest::qWait(10);

    QCOMPARE(spy.count(), 2);
}

void TestWavFormatChooser::widget_autoMode_combineVsExtract()
{
    m_widget->setAutoMode(WAVFormatChooserWidget::AutoMode::MaxFromInput);
    m_widget->reset();

    auto *sampleRateComboBox = m_widget->findChild<QComboBox *>("sampleRateComboBox");
    QVERIFY(sampleRateComboBox != nullptr);
    QVERIFY(sampleRateComboBox->itemText(0).contains("Auto"));

    m_widget->setAutoMode(WAVFormatChooserWidget::AutoMode::InheritFromInput);
    m_widget->reset();

    QVERIFY(sampleRateComboBox->itemText(0).contains("Inherit"));
}

void TestWavFormatChooser::widget_getFormat_complete()
{
    m_widget->reset();

    auto *sampleRateComboBox = m_widget->findChild<QComboBox *>("sampleRateComboBox");
    auto *sampleTypeComboBox = m_widget->findChild<QComboBox *>("sampleTypeComboBox");
    auto *channelsComboBox = m_widget->findChild<QComboBox *>("channelsComboBox");
    auto *containerComboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");

    sampleRateComboBox->setCurrentIndex(3);
    sampleTypeComboBox->setCurrentIndex(5);
    channelsComboBox->setCurrentIndex(3);
    containerComboBox->setCurrentIndex(0);
    QTest::qWait(10);

    AudioIO::AudioFormat format = m_widget->getFormat();

    QCOMPARE(format.kfr_format.samplerate, 48000.0);
    QCOMPARE(format.kfr_format.channels, 2);
    QCOMPARE(format.kfr_format.type, kfr::audio_sample_type::f32);
    QCOMPARE(format.container, AudioIO::AudioFormat::Container::RIFF);
}

void TestWavFormatChooser::widget_reset()
{
    auto *sampleRateComboBox = m_widget->findChild<QComboBox *>("sampleRateComboBox");
    auto *sampleTypeComboBox = m_widget->findChild<QComboBox *>("sampleTypeComboBox");
    auto *channelsComboBox = m_widget->findChild<QComboBox *>("channelsComboBox");
    auto *containerComboBox = m_widget->findChild<QComboBox *>("containerFormatComboBox");
    auto *spinBox = m_widget->findChild<QSpinBox *>("channelsSpinBox");

    sampleRateComboBox->setCurrentIndex(3);
    sampleTypeComboBox->setCurrentIndex(4);
    channelsComboBox->setCurrentIndex(5);
    containerComboBox->setCurrentIndex(1);
    QTest::qWait(10);

    m_widget->reset();

    QVERIFY(m_widget->isAutoSampleRate());
    QVERIFY(m_widget->isAutoSampleType());
    QVERIFY(m_widget->isAutoChannelCount());
    QCOMPARE(m_widget->getContainerFormat(), AudioIO::AudioFormat::Container::RIFF);
    QVERIFY(!spinBox->isVisible());
}

QTEST_MAIN(TestWavFormatChooser)
#include "test_wav_format_chooser.moc"
