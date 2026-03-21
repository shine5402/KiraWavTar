#include <QTest>
#include <utils/Utils.h>

class TestFilenames : public QObject
{
    Q_OBJECT

private slots:
    void getDescFileNameFrom_basic();
    void getDescFileNameFrom_flac();
    void getDescFileNameFrom_withPath();
    void getVolumeFileName_first();
    void getVolumeFileName_indexed();
    void getVolumeFileName_largeIndex();
};

void TestFilenames::getDescFileNameFrom_basic()
{
    QString result = utils::getDescFileNameFrom("/path/to/audio.wav");
    QVERIFY(result.endsWith(".kirawavtar-desc.json"));
    QVERIFY(result.contains("audio"));
}

void TestFilenames::getDescFileNameFrom_flac()
{
    QString result = utils::getDescFileNameFrom("/path/to/audio.flac");
    QVERIFY(result.endsWith(".kirawavtar-desc.json"));
    QVERIFY(result.contains("audio"));
}

void TestFilenames::getDescFileNameFrom_withPath()
{
#ifdef Q_OS_WIN
    QString input = "C:/Users/test/project/output.wav";
    QString expectedDir = "C:/Users/test/project";
#else
    QString input = "/home/user/project/output.wav";
    QString expectedDir = "/home/user/project";
#endif
    QString result = utils::getDescFileNameFrom(input);
    QCOMPARE(QFileInfo(result).fileName(), QString("output.kirawavtar-desc.json"));
    QCOMPARE(QFileInfo(result).absolutePath(), expectedDir);
}

void TestFilenames::getVolumeFileName_first()
{
    QString base = "/path/to/output.wav";
    QString result = utils::getVolumeFileName(base, 0);
    QCOMPARE(result, base);
}

void TestFilenames::getVolumeFileName_indexed()
{
    QString base = "/path/to/output.wav";
    QString result = utils::getVolumeFileName(base, 1);
    QVERIFY(result.contains("output.001.wav"));

    result = utils::getVolumeFileName(base, 2);
    QVERIFY(result.contains("output.002.wav"));
}

void TestFilenames::getVolumeFileName_largeIndex()
{
    QString base = "/path/to/output.wav";
    QString result = utils::getVolumeFileName(base, 99);
    QVERIFY(result.contains("output.099.wav"));

    result = utils::getVolumeFileName(base, 100);
    QVERIFY(result.contains("output.100.wav"));
}

QTEST_MAIN(TestFilenames)
#include "test_filenames.moc"
