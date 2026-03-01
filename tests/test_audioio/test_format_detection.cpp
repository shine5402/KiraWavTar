#include <QTest>
#include <worker/AudioIO.h>

class TestFormatDetection : public QObject
{
    Q_OBJECT

private slots:
    void readAudioFormat_wav();
    void readAudioFormat_wav_detectsSampleRate();
    void readAudioFormat_wav_detectsChannels();
    void readAudioFormat_wav_detectsBitDepth();
    void readAudioFormat_invalid_throws();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
};

void TestFormatDetection::readAudioFormat_wav()
{
    QString testFile = fixturesDir() + "/LJ001-0001.wav";
    QVERIFY(QFile::exists(testFile));

    AudioIO::AudioFormat format = AudioIO::readAudioFormat(testFile);
    QVERIFY(format.kfr_format.samplerate > 0);
    QVERIFY(format.kfr_format.channels > 0);
    QVERIFY(format.length > 0);
}

void TestFormatDetection::readAudioFormat_wav_detectsSampleRate()
{
    QString testFile = fixturesDir() + "/LJ001-0001.wav";
    AudioIO::AudioFormat format = AudioIO::readAudioFormat(testFile);

    QCOMPARE(format.kfr_format.samplerate, 22050.0);
}

void TestFormatDetection::readAudioFormat_wav_detectsChannels()
{
    QString testFile = fixturesDir() + "/LJ001-0001.wav";
    AudioIO::AudioFormat format = AudioIO::readAudioFormat(testFile);

    QCOMPARE(format.kfr_format.channels, 1u);
}

void TestFormatDetection::readAudioFormat_wav_detectsBitDepth()
{
    QString testFile = fixturesDir() + "/LJ001-0001.wav";
    AudioIO::AudioFormat format = AudioIO::readAudioFormat(testFile);

    QCOMPARE(format.kfr_format.type, kfr::audio_sample_type::i16);
}

void TestFormatDetection::readAudioFormat_invalid_throws()
{
    QString invalidFile = fixturesDir() + "/nonexistent.wav";
    QVERIFY_EXCEPTION_THROWN(AudioIO::readAudioFormat(invalidFile), std::runtime_error);
}

QTEST_MAIN(TestFormatDetection)
#include "test_format_detection.moc"
