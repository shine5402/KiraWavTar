#include <QTest>
#include <cli/CliUtils.h>

class TestCliUtils : public QObject
{
    Q_OBJECT

private slots:
    void stripHtml_plainText();
    void stripHtml_removesSimpleTags();
    void stripHtml_convertsBrToNewline();
    void stripHtml_convertsParagraphs();
    void stripHtml_removesStyleBlocks();
    void stripHtml_complexReport();
};

void TestCliUtils::stripHtml_plainText()
{
    QCOMPARE(cli::stripHtml("hello world"), QString("hello world"));
    QCOMPARE(cli::stripHtml(""), QString(""));
}

void TestCliUtils::stripHtml_removesSimpleTags()
{
    QCOMPARE(cli::stripHtml("<b>bold</b>"), QString("bold"));
    QCOMPARE(cli::stripHtml("<span class=\"x\">text</span>"), QString("text"));
    QCOMPARE(cli::stripHtml("<a href=\"url\">link</a>"), QString("link"));
}

void TestCliUtils::stripHtml_convertsBrToNewline()
{
    QCOMPARE(cli::stripHtml("line1<br>line2"), QString("line1\nline2"));
    QCOMPARE(cli::stripHtml("line1<br/>line2"), QString("line1\nline2"));
    QCOMPARE(cli::stripHtml("line1<br />line2"), QString("line1\nline2"));
}

void TestCliUtils::stripHtml_convertsParagraphs()
{
    QString result = cli::stripHtml("<p>para1</p><p>para2</p>");
    QVERIFY(result.contains("para1"));
    QVERIFY(result.contains("para2"));
    QVERIFY(result.contains("\n"));
}

void TestCliUtils::stripHtml_removesStyleBlocks()
{
    QCOMPARE(cli::stripHtml("<style>body{color:red}</style>text"), QString("text"));
    QCOMPARE(cli::stripHtml("<style type=\"text/css\">.x{font:bold}</style>content"), QString("content"));
}

void TestCliUtils::stripHtml_complexReport()
{
    QString html = "<style>.warn{color:orange}</style>"
                   "<p class=\"warn\">Warning: sample rate mismatch</p>"
                   "<p>File <b>test.wav</b> has rate 44100<br>Expected: 48000</p>";

    QString result = cli::stripHtml(html);
    QVERIFY(!result.contains("<"));
    QVERIFY(!result.contains(">"));
    QVERIFY(result.contains("Warning"));
    QVERIFY(result.contains("test.wav"));
    QVERIFY(result.contains("44100"));
}

QTEST_MAIN(TestCliUtils)
#include "test_cli_utils.moc"
