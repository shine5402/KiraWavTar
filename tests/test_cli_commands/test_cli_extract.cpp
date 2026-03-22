#include <QTest>
#include <QTemporaryDir>
#include <QDir>

#include <cli/ExtractCommand.h>
#include <cli/CombineCommand.h>
#include <utils/Utils.h>
#include <worker/AudioIO.h>

// TBB emit macro workaround
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

// Required by CombineCommand.cpp and ExtractCommand.cpp extern declarations
oneapi::tbb::task_group_context *g_tbbContext = nullptr;

class TestCliExtract : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void extract_basic();
    void extract_customDescFile();
    void extract_dryRun();
    void extract_filter();
    void extract_missingArgs();
    void extract_help();
    void extract_invalidFile();

private:
    QString fixturesDir() const { return QString::fromUtf8(TEST_FIXTURES_DIR); }
    QTemporaryDir m_tempDir;

    // Helper: combine fixtures into a single file for extraction tests
    bool combineFixtures(const QString &outputFile);
};

void TestCliExtract::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    utils::setSampleRateConversionQuality(kfr::sample_rate_conversion_quality::normal);
}

bool TestCliExtract::combineFixtures(const QString &outputFile)
{
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", fixturesDir(), outputFile});
    return rc == 0 && QFile::exists(outputFile);
}

void TestCliExtract::extract_basic()
{
    QString combinedFile = m_tempDir.path() + "/extract_basic.wav";
    QVERIFY(combineFixtures(combinedFile));

    QString outputDir = m_tempDir.path() + "/extracted_basic";
    int rc = cli::runExtract({"-q", combinedFile, outputDir});
    QCOMPARE(rc, 0);

    QDir dir(outputDir);
    QVERIFY(dir.exists());
    auto entries = dir.entryList(QDir::Files);
    QVERIFY2(!entries.isEmpty(), "Extracted files should exist");
}

void TestCliExtract::extract_customDescFile()
{
    QString combinedFile = m_tempDir.path() + "/extract_customdesc.wav";
    QString customDesc = m_tempDir.path() + "/extract_custom.json";

    // Combine with custom desc path
    int rc = cli::runCombine({"-q", "-ar", "22050", "-sf", "s16", "-ac", "1",
                              "--non-recursive", "--desc-filename", customDesc,
                              fixturesDir(), combinedFile});
    QCOMPARE(rc, 0);
    QVERIFY(QFile::exists(customDesc));

    QString outputDir = m_tempDir.path() + "/extracted_customdesc";
    rc = cli::runExtract({"-q", "--desc-file", customDesc, combinedFile, outputDir});
    QCOMPARE(rc, 0);

    QDir dir(outputDir);
    QVERIFY(dir.exists());
    QVERIFY(!dir.entryList(QDir::Files).isEmpty());
}

void TestCliExtract::extract_dryRun()
{
    QString combinedFile = m_tempDir.path() + "/extract_dryrun.wav";
    QVERIFY(combineFixtures(combinedFile));

    QString outputDir = m_tempDir.path() + "/extracted_dryrun";
    int rc = cli::runExtract({"-q", "--dry-run", combinedFile, outputDir});
    QCOMPARE(rc, 0);

    QDir dir(outputDir);
    QVERIFY2(!dir.exists() || dir.entryList(QDir::Files).isEmpty(),
             "Dry run should not create extracted files");
}

void TestCliExtract::extract_filter()
{
    QString combinedFile = m_tempDir.path() + "/extract_filter.wav";
    QVERIFY(combineFixtures(combinedFile));

    QString outputDir = m_tempDir.path() + "/extracted_filter";
    int rc = cli::runExtract({"-q", "--filter", "LJ001-0001*", combinedFile, outputDir});
    QCOMPARE(rc, 0);

    QDir dir(outputDir);
    auto entries = dir.entryList(QDir::Files);
    QCOMPARE(entries.size(), 1);
    QVERIFY(entries[0].startsWith("LJ001-0001"));
}

void TestCliExtract::extract_missingArgs()
{
    QCOMPARE(cli::runExtract({}), 2);
    QCOMPARE(cli::runExtract({"only-one-arg"}), 2);
}

void TestCliExtract::extract_help()
{
    QCOMPARE(cli::runExtract({"--help"}), 0);
}

void TestCliExtract::extract_invalidFile()
{
    QString outputDir = m_tempDir.path() + "/extracted_invalid";
    int rc = cli::runExtract({"-q", "/nonexistent/file.wav", outputDir});
    QCOMPARE(rc, 3);
}

QTEST_MAIN(TestCliExtract)
#include "test_cli_extract.moc"
