#include <QTest>
#include <QTemporaryDir>
#include <worker/AudioIO.h>
#include "../test_audio_compare.h"

class TestRoundtrip : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void roundtrip_wav_f32();
    void roundtrip_wav_i16();
    void roundtrip_wav_i24();
    void roundtrip_wav_i32();
    void roundtrip_preservesLength();
    void roundtrip_preservesSampleRate();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestRoundtrip::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestRoundtrip::cleanupTestCase()
{
}

void TestRoundtrip::roundtrip_wav_f32()
{
    QString inputFile = fixturesDir() + "/LJ001-0001.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_f32.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    QVERIFY(original.data.size() > 0);

    AudioIO::AudioFormat outputFormat = original.format;
    outputFormat.kfr_format.type = kfr::audio_sample_type::f32;
    outputFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioIO::writeAudioFileF32(outputFile, original.data, outputFormat);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.data.size(), original.data.size());
    QCOMPARE(roundtrip.format.kfr_format.channels, original.format.kfr_format.channels);

    auto result = AudioCompare::compareAudioBuffers(original.data, roundtrip.data, 0.999, 0.001);
    QVERIFY2(result.passed, result.message.toUtf8());
}

void TestRoundtrip::roundtrip_wav_i16()
{
    QString inputFile = fixturesDir() + "/LJ001-0001.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_i16.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    QVERIFY(original.data.size() > 0);

    AudioIO::AudioFormat outputFormat = original.format;
    outputFormat.kfr_format.type = kfr::audio_sample_type::i16;
    outputFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioIO::writeAudioFileF32(outputFile, original.data, outputFormat);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.data.size(), original.data.size());

    auto result = AudioCompare::compareAudioBuffers(original.data, roundtrip.data, 0.98, 0.02);
    QVERIFY2(result.passed, result.message.toUtf8());
}

void TestRoundtrip::roundtrip_wav_i24()
{
    QString inputFile = fixturesDir() + "/LJ001-0001.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_i24.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    QVERIFY(original.data.size() > 0);

    AudioIO::AudioFormat outputFormat = original.format;
    outputFormat.kfr_format.type = kfr::audio_sample_type::i24;
    outputFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioIO::writeAudioFileF32(outputFile, original.data, outputFormat);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.data.size(), original.data.size());

    auto result = AudioCompare::compareAudioBuffers(original.data, roundtrip.data, 0.99, 0.01);
    QVERIFY2(result.passed, result.message.toUtf8());
}

void TestRoundtrip::roundtrip_wav_i32()
{
    QString inputFile = fixturesDir() + "/LJ001-0001.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_i32.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    QVERIFY(original.data.size() > 0);

    AudioIO::AudioFormat outputFormat = original.format;
    outputFormat.kfr_format.type = kfr::audio_sample_type::i32;
    outputFormat.container = AudioIO::AudioFormat::Container::RIFF;

    AudioIO::writeAudioFileF32(outputFile, original.data, outputFormat);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.data.size(), original.data.size());

    auto result = AudioCompare::compareAudioBuffers(original.data, roundtrip.data, 0.999, 0.001);
    QVERIFY2(result.passed, result.message.toUtf8());
}

void TestRoundtrip::roundtrip_preservesLength()
{
    QString inputFile = fixturesDir() + "/LJ001-0005.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_length.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    QVERIFY(original.data.size() > 0);

    size_t originalLength = original.data[0].size();

    AudioIO::writeAudioFileF32(outputFile, original.data, original.format);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.data[0].size(), originalLength);
}

void TestRoundtrip::roundtrip_preservesSampleRate()
{
    QString inputFile = fixturesDir() + "/LJ001-0010.wav";
    QString outputFile = m_tempDir.path() + "/roundtrip_rate.wav";

    auto original = AudioIO::readAudioFileF32(inputFile);
    double originalRate = original.format.kfr_format.samplerate;

    AudioIO::writeAudioFileF32(outputFile, original.data, original.format);

    auto roundtrip = AudioIO::readAudioFileF32(outputFile);

    QCOMPARE(roundtrip.format.kfr_format.samplerate, originalRate);
}

QTEST_MAIN(TestRoundtrip)
#include "test_roundtrip.moc"
