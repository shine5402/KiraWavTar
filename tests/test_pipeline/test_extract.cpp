#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <worker/AudioExtract.h>
#include <worker/AudioCombine.h>
#include <worker/AudioIO.h>

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

class TestExtract : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void extract_basic();
    void extract_preservesContent();
    void extract_withResample();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestExtract::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestExtract::cleanupTestCase()
{
}

void TestExtract::extract_basic()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/extract_test.wav";
    QString outputDir = m_tempDir.path() + "/extracted";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QVERIFY(checkResult.pass != AudioCombine::CheckPassType::CRITICAL);

    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

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
    }
}

void TestExtract::extract_preservesContent()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/content_test.wav";
    QString outputDir = m_tempDir.path() + "/content_extracted";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::f32;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 2);

    QMap<QString, kfr::univector2d<float>> originalData;
    for (const auto &file : filesToCombine) {
        auto readResult = AudioIO::readAudioFileF32(file);
        originalData[file] = readResult.data;
    }

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

    for (const auto &file : filesToCombine) {
        QFileInfo srcInfo(file);
        QString extractedPath = outputDir + "/" + srcInfo.fileName();

        auto extractedData = AudioIO::readAudioFileF32(extractedPath).data;
        auto &original = originalData[file];

        QCOMPARE(extractedData.size(), original.size());

        size_t minLen = std::min(extractedData[0].size(), original[0].size());
        size_t matchingSamples = 0;
        for (size_t i = 0; i < minLen; ++i) {
            if (std::abs(extractedData[0][i] - original[0][i]) < 0.001f) {
                matchingSamples++;
            }
        }
        double matchRatio = static_cast<double>(matchingSamples) / minLen;
        QVERIFY2(matchRatio > 0.95,
                 QString("Content should be preserved, match ratio: %1").arg(matchRatio).toUtf8());
    }
}

void TestExtract::extract_withResample()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir.path() + "/resample_test.wav";
    QString outputDir = m_tempDir.path() + "/resample_extracted";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
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

QTEST_MAIN(TestExtract)
#include "test_extract.moc"
