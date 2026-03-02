#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>

#include <worker/AudioExtract.h>
#include <worker/AudioCombine.h>
#include <worker/AudioIO.h>

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

static float measureDCOffset(const kfr::univector<float> &samples)
{
    if (samples.empty())
        return 0.0f;
    double sum = 0.0;
    for (float s : samples) {
        sum += s;
    }
    return static_cast<float>(sum / samples.size());
}

static bool writeAudioFileWithDCOffset(const QString &filePath, double sampleRate, int channels,
                                        kfr::audio_sample_type sampleType, float dcOffset, int numSamples)
{
    kfr::univector2d<float> audioData(channels);
    for (int c = 0; c < channels; ++c) {
        audioData[c].resize(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float sine = std::sin(2.0f * 3.14159265f * 440.0f * t);
            audioData[c][i] = sine * 0.3f + dcOffset;
        }
    }

    AudioIO::AudioFormat format;
    format.kfr_format.samplerate = sampleRate;
    format.kfr_format.channels = channels;
    format.kfr_format.type = sampleType;
    format.container = AudioIO::AudioFormat::Container::RIFF;
    format.length = numSamples;

    return AudioIO::writeAudioFileF32(filePath, audioData, format);
}

class TestExtractConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void extract_inheritFormat_sameAsSource();
    void extract_inheritFormat_channelsInherited();
    void extract_inheritFormat_sampleRateInherited();
    void extract_manualFormat_override();
    void extract_removeDCOffset();
    void extract_resampleOnExtract();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestExtractConfig::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestExtractConfig::cleanupTestCase()
{
}

void TestExtractConfig::extract_inheritFormat_sameAsSource()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/inherit_same.wav";
    QString outputDir = m_tempDir.path() + "/inherit_same_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    QMap<QString, AudioIO::AudioFormat> originalFormats;
    for (const auto &file : filesToCombine) {
        originalFormats[file] = AudioIO::readAudioFormat(file);
    }

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, targetFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);
    QCOMPARE(extractCheck.pass, AudioExtract::CheckPassType::OK);

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.removeDCOffset = false;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    for (const auto &file : filesToCombine) {
        QFileInfo srcInfo(file);
        QString extractedPath = outputDir + "/" + srcInfo.fileName();

        QVERIFY2(QFile::exists(extractedPath),
                 QString("Extracted file should exist: %1").arg(extractedPath).toUtf8());

        auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
        auto &originalFormat = originalFormats[file];

        QCOMPARE(extractedFormat.kfr_format.samplerate, targetFormat.kfr_format.samplerate);
        QCOMPARE(extractedFormat.kfr_format.channels, targetFormat.kfr_format.channels);
        QCOMPARE(extractedFormat.kfr_format.type, targetFormat.kfr_format.type);
    }
}

void TestExtractConfig::extract_inheritFormat_channelsInherited()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/inherit_channels.wav";
    QString outputDir = m_tempDir.path() + "/inherit_channels_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 2;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, targetFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.removeDCOffset = false;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    auto descriptions = extractCheck.descRoot["descriptions"].toArray();
    for (int i = 0; i < descriptions.size(); ++i) {
        auto desc = descriptions[i].toObject();
        int expectedChannels = desc["channel_count"].toInt();

        QString extractedPath = outputDir + "/" + desc["file_name"].toString();
        if (QFile::exists(extractedPath)) {
            auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
            QCOMPARE(extractedFormat.kfr_format.channels, expectedChannels);
        }
    }
}

void TestExtractConfig::extract_inheritFormat_sampleRateInherited()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/inherit_samplerate.wav";
    QString outputDir = m_tempDir.path() + "/inherit_samplerate_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 44100.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, targetFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.removeDCOffset = false;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    auto descriptions = extractCheck.descRoot["descriptions"].toArray();
    for (int i = 0; i < descriptions.size(); ++i) {
        auto desc = descriptions[i].toObject();
        double expectedSampleRate = desc["sample_rate"].toDouble();

        QString extractedPath = outputDir + "/" + desc["file_name"].toString();
        if (QFile::exists(extractedPath)) {
            auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
            QCOMPARE(extractedFormat.kfr_format.samplerate, expectedSampleRate);
        }
    }
}

void TestExtractConfig::extract_manualFormat_override()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/manual_override.wav";
    QString outputDir = m_tempDir.path() + "/manual_override_out";

    AudioIO::AudioFormat combineFormat;
    combineFormat.kfr_format.samplerate = 22050.0;
    combineFormat.kfr_format.channels = 1;
    combineFormat.kfr_format.type = kfr::audio_sample_type::i16;
    combineFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, combineFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, combineFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioIO::AudioFormat extractFormat;
    extractFormat.kfr_format.samplerate = 48000.0;
    extractFormat.kfr_format.channels = 2;
    extractFormat.kfr_format.type = kfr::audio_sample_type::f32;
    extractFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.targetFormat = extractFormat;
    params.removeDCOffset = false;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    for (const auto &file : filesToCombine) {
        QFileInfo srcInfo(file);
        QString extractedPath = outputDir + "/" + srcInfo.fileName();

        QVERIFY2(QFile::exists(extractedPath),
                 QString("Extracted file should exist: %1").arg(extractedPath).toUtf8());

        auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
        QCOMPARE(extractedFormat.kfr_format.samplerate, 48000.0);
        QCOMPARE(extractedFormat.kfr_format.channels, 2);
        QCOMPARE(extractedFormat.kfr_format.type, kfr::audio_sample_type::f32);
    }
}

void TestExtractConfig::extract_removeDCOffset()
{
    QString dcOffsetFile = m_tempDir.path() + "/dc_offset_source.wav";
    QString combinedFile = m_tempDir.path() + "/dc_combined.wav";
    QString outputDir = m_tempDir.path() + "/dc_output";

    float dcOffset = 0.2f;
    int numSamples = 22050;
    QVERIFY(writeAudioFileWithDCOffset(dcOffsetFile, 22050.0, 1, kfr::audio_sample_type::f32, dcOffset, numSamples));

    auto originalData = AudioIO::readAudioFileF32(dcOffsetFile).data;
    float originalDC = measureDCOffset(originalData[0]);
    QVERIFY2(std::abs(originalDC - dcOffset) < 0.01f,
             QString("Original DC offset should be ~0.2, got: %1").arg(originalDC).toUtf8());

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::f32;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    QStringList filesToCombine = {dcOffsetFile};
    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, m_tempDir.path(), combinedFile, targetFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.removeDCOffset = true;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    QString extractedPath = outputDir + "/dc_offset_source.wav";
    QVERIFY(QFile::exists(extractedPath));

    auto extractedData = AudioIO::readAudioFileF32(extractedPath).data;
    float extractedDC = measureDCOffset(extractedData[0]);

    QVERIFY2(std::abs(extractedDC) < std::abs(originalDC) * 0.1,
             QString("DC offset should be reduced by at least 90%%. Original: %1, After: %2")
                 .arg(originalDC)
                 .arg(extractedDC)
                 .toUtf8());
}

void TestExtractConfig::extract_resampleOnExtract()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/resample_combined.wav";
    QString outputDir = m_tempDir.path() + "/resample_output";

    AudioIO::AudioFormat combineFormat;
    combineFormat.kfr_format.samplerate = 22050.0;
    combineFormat.kfr_format.channels = 1;
    combineFormat.kfr_format.type = kfr::audio_sample_type::i16;
    combineFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, combineFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, combineFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioIO::AudioFormat extractFormat;
    extractFormat.kfr_format.samplerate = 44100.0;
    extractFormat.kfr_format.channels = 1;
    extractFormat.kfr_format.type = kfr::audio_sample_type::i16;
    extractFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = combinedFile;
    params.descRoot = extractCheck.descRoot;
    params.dstDirName = outputDir;
    params.targetFormat = extractFormat;
    params.removeDCOffset = false;
    params.gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    params.filteredDescArray = extractCheck.descRoot["descriptions"].toArray();

    std::atomic<int> extractProgress{0};
    oneapi::tbb::task_group_context extractCtx;
    auto result = AudioExtract::runExtractPipeline(params, extractProgress, extractCtx);

    QVERIFY(result.errors.isEmpty());

    for (const auto &file : filesToCombine) {
        QFileInfo srcInfo(file);
        QString extractedPath = outputDir + "/" + srcInfo.fileName();

        auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
        QCOMPARE(extractedFormat.kfr_format.samplerate, 44100.0);
    }
}

QTEST_MAIN(TestExtractConfig)
#include "test_extract_config.moc"
