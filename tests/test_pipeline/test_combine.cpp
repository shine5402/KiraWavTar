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

class TestCombine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void combine_basic();
    void combine_withGap();
    void combine_formatConversion();
    void combine_createsDescriptionFile();
    void combine_descriptionContainsMetadata();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestCombine::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestCombine::cleanupTestCase()
{
}

void TestCombine::combine_basic()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_basic.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);
    QVERIFY(checkResult.wavFileNames.size() >= 3);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    QCOMPARE(layout.entries.size(), 3);
    QCOMPARE(layout.volumes.size(), 1);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    auto descJson = AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(QFile::exists(outputFile));
    QVERIFY(descJson.contains("descriptions"));

    auto combinedFormat = AudioIO::readAudioFormat(outputFile);
    QVERIFY(combinedFormat.length > 0);
}

void TestCombine::combine_withGap()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_gap.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    int gapMs = 100;
    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, gapMs, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    auto combinedFormat = AudioIO::readAudioFormat(outputFile);
    qint64 expectedGapSamples = static_cast<qint64>(gapMs / 1000.0 * targetFormat.kfr_format.samplerate);

    qint64 totalInputSamples = 0;
    for (const auto &file : filesToCombine) {
        auto fmt = AudioIO::readAudioFormat(file);
        totalInputSamples += fmt.length;
    }

    qint64 expectedTotal = totalInputSamples + 2 * expectedGapSamples * filesToCombine.size();
    qint64 actualTotal = combinedFormat.length;

    QVERIFY2(actualTotal >= totalInputSamples,
             "Combined file should be at least as long as sum of inputs");
}

void TestCombine::combine_formatConversion()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_f32.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::f32;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    auto combinedFormat = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(combinedFormat.kfr_format.type, kfr::audio_sample_type::f32);
}

void TestCombine::combine_createsDescriptionFile()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_desc.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    AudioCombine::runCombinePipeline(layout, progress, ctx);

    QString descFile = utils::getDescFileNameFrom(outputFile);
    QVERIFY2(QFile::exists(descFile), "Description file should be created");
}

void TestCombine::combine_descriptionContainsMetadata()
{
    QString inputDir = fixturesDir();
    QString outputFile = m_tempDir.path() + "/combined_meta.wav";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, outputFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, outputFile, targetFormat, 0, volumeConfig);

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    auto descJson = AudioCombine::runCombinePipeline(layout, progress, ctx);

    QVERIFY(descJson.contains("version"));
    QVERIFY(descJson.contains("sample_rate"));
    QVERIFY(descJson.contains("sample_type"));
    QVERIFY(descJson.contains("channel_count"));
    QVERIFY(descJson.contains("descriptions"));

    QCOMPARE(descJson["version"].toInt(), utils::desc_file_version);
    QCOMPARE(descJson["sample_rate"].toDouble(), 22050.0);

    auto descriptions = descJson["descriptions"].toArray();
    QCOMPARE(descriptions.size(), 2);

    for (const auto &desc : descriptions) {
        auto obj = desc.toObject();
        QVERIFY(obj.contains("file_name"));
        QVERIFY(obj.contains("begin_time"));
        QVERIFY(obj.contains("duration"));
        QVERIFY(obj.contains("sample_rate"));
        QVERIFY(obj.contains("sample_type"));
    }
}

QTEST_MAIN(TestCombine)
#include "test_combine.moc"
