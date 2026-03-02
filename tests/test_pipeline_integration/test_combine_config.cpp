#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>

#include <worker/AudioCombine.h>
#include <worker/AudioIO.h>

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

class TestCombineConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void combine_manualFormat_44100_stereo_i16();
    void combine_manualFormat_48000_mono_f32();
    void combine_containerFormat_flac();
    void combine_containerFormat_rf64();
    void combine_containerFormat_w64();
    void combine_withGap_50ms();
    void combine_withGap_200ms();
    void combine_volumeSplit_byCount();
    void combine_volumeSplit_byDuration();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestCombineConfig::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestCombineConfig::cleanupTestCase()
{
}

void TestCombineConfig::combine_manualFormat_44100_stereo_i16()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_44100_stereo_i16.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 44100.0;
    targetFormat.kfr_format.channels = 2;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));

    auto outputFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(outputFormat.kfr_format.samplerate, 44100.0);
    QCOMPARE(outputFormat.kfr_format.channels, 2);
    QCOMPARE(outputFormat.kfr_format.type, kfr::audio_sample_type::i16);
    QCOMPARE(outputFormat.container, AudioIO::AudioFormat::Container::RIFF);
}

void TestCombineConfig::combine_manualFormat_48000_mono_f32()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_48000_mono_f32.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 48000.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::f32;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));

    auto outputFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(outputFormat.kfr_format.samplerate, 48000.0);
    QCOMPARE(outputFormat.kfr_format.channels, 1);
    QCOMPARE(outputFormat.kfr_format.type, kfr::audio_sample_type::f32);
    QCOMPARE(outputFormat.container, AudioIO::AudioFormat::Container::RIFF);
}

void TestCombineConfig::combine_containerFormat_flac()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_flac.flac";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::FLAC;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));

    auto outputFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(outputFormat.container, AudioIO::AudioFormat::Container::FLAC);
    QVERIFY(outputFormat.kfr_format.type == kfr::audio_sample_type::i16 ||
            outputFormat.kfr_format.type == kfr::audio_sample_type::i24 ||
            outputFormat.kfr_format.type == kfr::audio_sample_type::i32);
}

void TestCombineConfig::combine_containerFormat_rf64()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_rf64.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RF64;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));

    auto outputFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(outputFormat.container, AudioIO::AudioFormat::Container::RF64);
}

void TestCombineConfig::combine_containerFormat_w64()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_w64.w64";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::W64;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));

    auto outputFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(outputFormat.container, AudioIO::AudioFormat::Container::W64);
}

void TestCombineConfig::combine_withGap_50ms()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_gap_50ms.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    qint64 totalInputSamples = 0;
    for (const auto &file : filesToCombine) {
        auto fmt = AudioIO::readAudioFormat(file);
        totalInputSamples += fmt.length;
    }

    int gapMs = 50;
    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, gapMs, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    auto combinedFormat = AudioIO::readAudioFormat(outputFile);
    qint64 expectedGapSamples = static_cast<qint64>(gapMs / 1000.0 * targetFormat.kfr_format.samplerate);
    qint64 expectedTotal = totalInputSamples + expectedGapSamples * (filesToCombine.size() - 1);

    QVERIFY2(combinedFormat.length >= totalInputSamples,
             "Combined file should be at least as long as sum of inputs");
    QVERIFY2(combinedFormat.length >= expectedTotal - expectedGapSamples,
             "Combined file should include gap samples");
}

void TestCombineConfig::combine_withGap_200ms()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_gap_200ms.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    qint64 totalInputSamples = 0;
    for (const auto &file : filesToCombine) {
        auto fmt = AudioIO::readAudioFormat(file);
        totalInputSamples += fmt.length;
    }

    int gapMs = 200;
    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, gapMs, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    auto combinedFormat = AudioIO::readAudioFormat(outputFile);
    qint64 expectedGapSamples = static_cast<qint64>(gapMs / 1000.0 * targetFormat.kfr_format.samplerate);
    qint64 expectedTotal = totalInputSamples + expectedGapSamples * (filesToCombine.size() - 1);

    QVERIFY2(combinedFormat.length >= expectedTotal - expectedGapSamples,
             "Combined file should include 200ms gap samples");
}

void TestCombineConfig::combine_volumeSplit_byCount()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_volume_count.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.wavFileNames.size() >= 5);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 5);

    utils::VolumeConfig volumeConfig;
    volumeConfig.mode = utils::VolumeSplitMode::ByCount;
    volumeConfig.maxEntriesPerVolume = 2;

    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    QCOMPARE(layout.volumes.size(), 3);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QString descFile = utils::getDescFileNameFrom(outputFile);
    QVERIFY(QFile::exists(descFile));

    QFile f(descFile);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QVERIFY(doc.object().contains("volume_count"));
    QCOMPARE(doc.object()["volume_count"].toInt(), 3);
    QVERIFY(doc.object().contains("volumes"));

    for (int i = 0; i < 3; ++i) {
        QString volumeFile = utils::getVolumeFileName(outputFile, i);
        QVERIFY2(QFile::exists(volumeFile), QString("Volume file %1 should exist").arg(volumeFile).toUtf8());
    }
}

void TestCombineConfig::combine_volumeSplit_byDuration()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_volume_duration.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames;

    utils::VolumeConfig volumeConfig;
    volumeConfig.mode = utils::VolumeSplitMode::ByDuration;
    volumeConfig.maxDurationSeconds = 10;

    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    QVERIFY(layout.volumes.size() >= 1);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    for (int i = 0; i < layout.volumes.size(); ++i) {
        QString volumeFile = utils::getVolumeFileName(outputFile, i);
        QVERIFY2(QFile::exists(volumeFile), QString("Volume file %1 should exist").arg(volumeFile).toUtf8());

        auto format = AudioIO::readAudioFormat(volumeFile);
        double durationSeconds = static_cast<double>(format.length) / format.kfr_format.samplerate;
        QVERIFY2(durationSeconds <= 12.0,
                 QString("Volume %1 duration should be close to limit: %2s")
                     .arg(i)
                     .arg(durationSeconds)
                     .toUtf8());
    }
}

QTEST_MAIN(TestCombineConfig)
#include "test_combine_config.moc"
