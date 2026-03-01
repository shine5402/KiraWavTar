#include <QTest>
#include <utils/Utils.h>

class TestTimecode : public QObject
{
    Q_OBJECT

private slots:
    void samplesToTimecode_basic();
    void samplesToTimecode_zero();
    void samplesToTimecode_large();
    void samplesToTimecode_fractional();
    void timecodeToSamples_basic();
    void timecodeToSamples_roundtrip();
    void timecodeToSamples_empty();
    void timecodeToSamples_invalid();
    void timecodeToSeconds_basic();
    void timecodeToSeconds_empty();
};

void TestTimecode::samplesToTimecode_basic()
{
    QCOMPARE(utils::samplesToTimecode(44100, 44100.0), QString("00:00:01.000"));
    QCOMPARE(utils::samplesToTimecode(22050, 22050.0), QString("00:00:01.000"));
    QCOMPARE(utils::samplesToTimecode(48000, 48000.0), QString("00:00:01.000"));
    QCOMPARE(utils::samplesToTimecode(0, 44100.0), QString("00:00:00.000"));
}

void TestTimecode::samplesToTimecode_zero()
{
    QCOMPARE(utils::samplesToTimecode(0, 44100.0), QString("00:00:00.000"));
    QCOMPARE(utils::samplesToTimecode(0, 48000.0), QString("00:00:00.000"));
}

void TestTimecode::samplesToTimecode_large()
{
    qint64 oneHourSamples = static_cast<qint64>(44100.0 * 3600.0);
    QString result = utils::samplesToTimecode(oneHourSamples, 44100.0);
    QVERIFY(result.startsWith("01:00:00"));

    qint64 twoHours = static_cast<qint64>(48000.0 * 7200.0);
    result = utils::samplesToTimecode(twoHours, 48000.0);
    QVERIFY(result.startsWith("02:00:00"));
}

void TestTimecode::samplesToTimecode_fractional()
{
    QString result = utils::samplesToTimecode(441, 44100.0);
    QCOMPARE(result, QString("00:00:00.010"));

    result = utils::samplesToTimecode(2205, 22050.0);
    QCOMPARE(result, QString("00:00:00.100"));

    result = utils::samplesToTimecode(1102, 22050.0);
    QVERIFY(result.startsWith("00:00:00.050"));
}

void TestTimecode::timecodeToSamples_basic()
{
    QCOMPARE(utils::timecodeToSamples("00:00:01.000", 44100.0), 44100LL);
    QCOMPARE(utils::timecodeToSamples("00:00:01.000", 48000.0), 48000LL);
    QCOMPARE(utils::timecodeToSamples("00:00:00.000", 44100.0), 0LL);
    QCOMPARE(utils::timecodeToSamples("00:01:00.000", 44100.0), 2646000LL);
    QCOMPARE(utils::timecodeToSamples("01:00:00.000", 44100.0), 158760000LL);
}

void TestTimecode::timecodeToSamples_roundtrip()
{
    QList<double> sampleRates = {22050.0, 44100.0, 48000.0, 96000.0};
    QList<qint64> testSamples = {0, 1000, 44100, 48000, 2646000, 158760000};

    for (double rate : sampleRates) {
        for (qint64 samples : testSamples) {
            QString timecode = utils::samplesToTimecode(samples, rate);
            qint64 converted = utils::timecodeToSamples(timecode, rate);
            qint64 maxError = static_cast<qint64>(rate / 1000.0) + 1;
            QVERIFY2(qAbs(converted - samples) <= maxError,
                     QString("Roundtrip failed for rate=%1, samples=%2: got %3, maxError=%4")
                         .arg(rate)
                         .arg(samples)
                         .arg(converted)
                         .arg(maxError)
                         .toUtf8());
        }
    }
}

void TestTimecode::timecodeToSamples_empty()
{
    QCOMPARE(utils::timecodeToSamples("", 44100.0), 0LL);
}

void TestTimecode::timecodeToSamples_invalid()
{
    QCOMPARE(utils::timecodeToSamples("invalid", 44100.0), 0LL);
    QCOMPARE(utils::timecodeToSamples("00:00", 44100.0), 0LL);
    QCOMPARE(utils::timecodeToSamples("00:00:00", 44100.0), 0LL);
}

void TestTimecode::timecodeToSeconds_basic()
{
    QCOMPARE(utils::timecodeToSeconds("00:00:00.000"), 0.0);
    QCOMPARE(utils::timecodeToSeconds("00:00:01.000"), 1.0);
    QCOMPARE(utils::timecodeToSeconds("00:01:00.000"), 60.0);
    QCOMPARE(utils::timecodeToSeconds("01:00:00.000"), 3600.0);
    QCOMPARE(utils::timecodeToSeconds("01:30:45.500"), 5445.5);
}

void TestTimecode::timecodeToSeconds_empty()
{
    QCOMPARE(utils::timecodeToSeconds(""), 0.0);
}

QTEST_MAIN(TestTimecode)
#include "test_timecode.moc"
