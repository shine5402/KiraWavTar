#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>

#include <cli/CombineCommand.h>
#include <utils/Utils.h>
#include <worker/AudioIO.h>

// TBB emit macro workaround
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

// Required by CombineCommand.cpp's extern declaration
oneapi::tbb::task_group_context *g_tbbContext = nullptr;

class TestCliCombine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void combine_basic();
    void combine_noDescFile();
    void combine_customDescFile();
    void combine_dryRun();
    void combine_formatOptions();
    void combine_missingArgs();
    void combine_unknownOption();
    void combine_help();
    void combine_invalidInput();
    void combine_conflictingDescOptions();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;
};

void TestCliCombine::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

void TestCliCombine::combine_basic()
{
    QString outputFile = m_tempDir.path() + "/basic.wav";
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", fixturesDir(), outputFile});
    QCOMPARE(rc, 0);
    QVERIFY(QFile::exists(outputFile));

    QString descFile = utils::getDescFileNameFrom(outputFile);
    QVERIFY2(QFile::exists(descFile), "Description file should be created by default");
}

void TestCliCombine::combine_noDescFile()
{
    QString outputFile = m_tempDir.path() + "/nodesc.wav";
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", "--no-desc-file", fixturesDir(), outputFile});
    QCOMPARE(rc, 0);
    QVERIFY(QFile::exists(outputFile));

    QString descFile = utils::getDescFileNameFrom(outputFile);
    QVERIFY2(!QFile::exists(descFile), "Description file should NOT be created with --no-desc-file");
}

void TestCliCombine::combine_customDescFile()
{
    QString outputFile = m_tempDir.path() + "/customdesc.wav";
    QString customDesc = m_tempDir.path() + "/custom.json";
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", "--desc-filename", customDesc,
                              fixturesDir(), outputFile});
    QCOMPARE(rc, 0);
    QVERIFY(QFile::exists(outputFile));
    QVERIFY2(QFile::exists(customDesc), "Custom description file should exist");

    QString defaultDesc = utils::getDescFileNameFrom(outputFile);
    QVERIFY2(!QFile::exists(defaultDesc), "Default desc path should NOT exist");
}

void TestCliCombine::combine_dryRun()
{
    QString outputFile = m_tempDir.path() + "/dryrun.wav";
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", "--dry-run", fixturesDir(), outputFile});
    QCOMPARE(rc, 0);
    QVERIFY2(!QFile::exists(outputFile), "Dry run should not create output file");
}

void TestCliCombine::combine_formatOptions()
{
    QString outputFile = m_tempDir.path() + "/format.wav";
    int rc = cli::runCombine({"-q", "-ar", "48000", "-sf", "f32", "-ac", "1", "-f", "riff",
                              "--non-recursive", fixturesDir(), outputFile});
    QCOMPARE(rc, 0);
    QVERIFY(QFile::exists(outputFile));

    auto format = AudioIO::readAudioFormat(outputFile);
    QCOMPARE(format.kfr_format.samplerate, 48000.0);
    QCOMPARE(format.kfr_format.type, kfr::audio_sample_type::f32);
}

void TestCliCombine::combine_missingArgs()
{
    QCOMPARE(cli::runCombine({}), 2);
    QCOMPARE(cli::runCombine({"only-one-arg"}), 2);
}

void TestCliCombine::combine_unknownOption()
{
    QCOMPARE(cli::runCombine({"--bogus", "a", "b"}), 2);
}

void TestCliCombine::combine_help()
{
    QCOMPARE(cli::runCombine({"--help"}), 0);
}

void TestCliCombine::combine_invalidInput()
{
    QString outputFile = m_tempDir.path() + "/invalid.wav";
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "/nonexistent/path/that/does/not/exist", outputFile});
    QCOMPARE(rc, 3);
}

void TestCliCombine::combine_conflictingDescOptions()
{
    int rc = cli::runCombine({"--no-desc-file", "--desc-filename", "x.json",
                              fixturesDir(), m_tempDir.path() + "/conflict.wav"});
    QCOMPARE(rc, 2);
}

QTEST_MAIN(TestCliCombine)
#include "test_cli_combine.moc"
