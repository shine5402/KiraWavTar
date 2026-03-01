#include <QTest>
#include <QDir>
#include <QStandardPaths>
#include <worker/AudioExtract.h>
#include <worker/AudioCombine.h>
#include <worker/AudioIO.h>
#include "../test_audio_compare.h"

#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

class TestRoundtripFull : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void roundtrip_wav_sameFormat();
    void roundtrip_wav_formatChange();
    void roundtrip_multipleFiles();
    void roundtrip_withGap();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QString m_tempDir;
};

void TestRoundtripFull::initTestCase()
{
    m_tempDir = QDir::tempPath() + "/KiraWavTar_test_roundtrip";
    QDir().mkpath(m_tempDir);
    QVERIFY(QDir(m_tempDir).exists());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestRoundtripFull::cleanupTestCase()
{
}

void TestRoundtripFull::roundtrip_wav_sameFormat()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir + "/roundtrip_same.wav";
    QString outputDir = m_tempDir + "/roundtrip_same_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QVERIFY(checkResult.wavFileNames.size() > 0);

    QString testFile = checkResult.wavFileNames.first();
    auto originalData = AudioIO::readAudioFileF32(testFile).data;

    QStringList filesToCombine = {testFile};

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

    QFileInfo srcInfo(testFile);
    QString extractedPath = outputDir + "/" + srcInfo.fileName();

    auto extractedData = AudioIO::readAudioFileF32(extractedPath).data;

    auto comparison = AudioCompare::compareAudioBuffers(originalData, extractedData, 0.99, 0.01);
    QVERIFY2(comparison.passed,
             QString("Roundtrip should preserve audio content: %1").arg(comparison.message).toUtf8());
}

void TestRoundtripFull::roundtrip_wav_formatChange()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir + "/roundtrip_format.wav";
    QString outputDir = m_tempDir + "/roundtrip_format_out";

    AudioIO::AudioFormat combineFormat;
    combineFormat.kfr_format.samplerate = 22050.0;
    combineFormat.kfr_format.channels = 1;
    combineFormat.kfr_format.type = kfr::audio_sample_type::f32;
    combineFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, combineFormat);
    QString testFile = checkResult.wavFileNames.first();

    auto originalData = AudioIO::readAudioFileF32(testFile).data;

    QStringList filesToCombine = {testFile};

    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, combineFormat, 0, volumeConfig);

    std::atomic<int> combineProgress{0};
    oneapi::tbb::task_group_context combineCtx;
    AudioCombine::runCombinePipeline(layout, combineProgress, combineCtx);

    auto extractCheck = AudioExtract::preCheck(combinedFile, outputDir);

    AudioIO::AudioFormat extractFormat;
    extractFormat.kfr_format.samplerate = 22050.0;
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

    QFileInfo srcInfo(testFile);
    QString extractedPath = outputDir + "/" + srcInfo.fileName();

    auto extractedFormat = AudioIO::readAudioFormat(extractedPath);
    QCOMPARE(extractedFormat.kfr_format.type, kfr::audio_sample_type::i16);

    auto extractedData = AudioIO::readAudioFileF32(extractedPath).data;

    auto comparison = AudioCompare::compareAudioBuffers(originalData, extractedData, 0.95, 0.02);
    QVERIFY2(comparison.passed,
             QString("Roundtrip with format change should preserve audio reasonably: %1")
                 .arg(comparison.message)
                 .toUtf8());
}

void TestRoundtripFull::roundtrip_multipleFiles()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir + "/roundtrip_multi.wav";
    QString outputDir = m_tempDir + "/roundtrip_multi_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 5);

    QMap<QString, kfr::univector2d<float>> originalDataMap;
    for (const auto &file : filesToCombine) {
        originalDataMap[file] = AudioIO::readAudioFileF32(file).data;
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

        QVERIFY2(QFile::exists(extractedPath),
                 QString("Extracted file should exist: %1").arg(extractedPath).toUtf8());

        auto extractedData = AudioIO::readAudioFileF32(extractedPath).data;
        auto &originalData = originalDataMap[file];

        auto comparison = AudioCompare::compareAudioBuffers(originalData, extractedData, 0.99, 0.01);
        QVERIFY2(comparison.passed,
                 QString("File %1 should be preserved: %2").arg(srcInfo.fileName(), comparison.message).toUtf8());
    }
}

void TestRoundtripFull::roundtrip_withGap()
{
    QString inputDir = fixturesDir();
    QString combinedFile = m_tempDir + "/roundtrip_gap.wav";
    QString outputDir = m_tempDir + "/roundtrip_gap_out";

    AudioIO::AudioFormat targetFormat;
    targetFormat.kfr_format.samplerate = 22050.0;
    targetFormat.kfr_format.channels = 1;
    targetFormat.kfr_format.type = kfr::audio_sample_type::i16;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF;

    auto checkResult = AudioCombine::preCheck(inputDir, combinedFile, false, targetFormat);
    QStringList filesToCombine = checkResult.wavFileNames.mid(0, 3);

    QMap<QString, kfr::univector2d<float>> originalDataMap;
    for (const auto &file : filesToCombine) {
        originalDataMap[file] = AudioIO::readAudioFileF32(file).data;
    }

    int gapMs = 50;
    utils::VolumeConfig volumeConfig;
    auto layout = AudioCombine::computeLayout(filesToCombine, inputDir, combinedFile, targetFormat, gapMs, volumeConfig);

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
        auto &originalData = originalDataMap[file];

        auto comparison = AudioCompare::compareAudioBuffers(originalData, extractedData, 0.99, 0.01);
        QVERIFY2(comparison.passed,
                 QString("File %1 should be preserved with gap: %2").arg(srcInfo.fileName(), comparison.message).toUtf8());
    }
}

QTEST_MAIN(TestRoundtripFull)
#include "test_roundtrip_full.moc"
