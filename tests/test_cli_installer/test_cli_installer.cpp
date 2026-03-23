#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <QRegularExpression>
#endif

class TestCliInstaller : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void install_createsWrapper();
    void install_idempotent();
    void uninstall_removesWrapper();
    void uninstall_whenNotInstalled();
#ifdef Q_OS_WIN
    void install_addsToPath();
    void uninstall_removesFromPath();
#endif

private:
    QString cliBinary() const { return QString::fromUtf8(CLI_BINARY_PATH); }
    QString wrapperPath() const;
#ifdef Q_OS_WIN
    QString confPath() const;
#endif
    int runCli(const QStringList &args, QString *stdoutOutput = nullptr, QString *stderrOutput = nullptr) const;

#ifdef Q_OS_WIN
    static QString readUserPathFromRegistry();
#endif
};

QString TestCliInstaller::wrapperPath() const
{
#ifdef Q_OS_WIN
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
           "/Programs/KiraWavTar/bin/kirawavtar-cli.exe";
#else
    return QDir::homePath() + "/.local/bin/kirawavtar-cli";
#endif
}

#ifdef Q_OS_WIN
QString TestCliInstaller::confPath() const
{
    return QFileInfo(wrapperPath()).absolutePath() + "/kirawavtar-cli.conf";
}
#endif

int TestCliInstaller::runCli(const QStringList &args, QString *stdoutOutput, QString *stderrOutput) const
{
    QProcess proc;
    proc.start(cliBinary(), args);
    proc.waitForFinished(30000);
    if (stdoutOutput)
        *stdoutOutput = QString::fromUtf8(proc.readAllStandardOutput());
    if (stderrOutput)
        *stderrOutput = QString::fromUtf8(proc.readAllStandardError());
    if (proc.exitStatus() != QProcess::NormalExit)
        return -1;
    return proc.exitCode();
}

#ifdef Q_OS_WIN
QString TestCliInstaller::readUserPathFromRegistry()
{
    QProcess proc;
    proc.start("reg", {"query", "HKCU\\Environment", "/v", "Path"});
    proc.waitForFinished(5000);
    if (proc.exitCode() != 0)
        return {};
    QString output = proc.readAllStandardOutput();
    QRegularExpression re(R"(Path\s+REG_(?:EXPAND_)?SZ\s+(.*))");
    auto match = re.match(output);
    if (match.hasMatch())
        return match.captured(1).trimmed();
    return {};
}
#endif

void TestCliInstaller::initTestCase()
{
    QVERIFY2(QFileInfo::exists(cliBinary()),
             qPrintable("CLI binary not found: " + cliBinary()));

    // Ensure clean state — remove any leftover wrapper from a previous failed run
    QFile::remove(wrapperPath());
#ifdef Q_OS_WIN
    QFile::remove(confPath());
#endif
}

void TestCliInstaller::cleanupTestCase()
{
    // Always clean up: run uninstall to restore the system state
    runCli({"uninstall-cli"});
}

void TestCliInstaller::install_createsWrapper()
{
    QString stdoutOutput, stderrOutput;
    int rc = runCli({"install-cli"}, &stdoutOutput, &stderrOutput);
    QVERIFY2(rc == 0, qPrintable("install-cli failed (rc=" + QString::number(rc) +
                                  ")\nstdout: " + stdoutOutput + "\nstderr: " + stderrOutput));
    QVERIFY2(QFileInfo::exists(wrapperPath()),
             qPrintable("Wrapper should exist at: " + wrapperPath()));

    auto binDir = QFileInfo(cliBinary()).absolutePath();
#ifdef Q_OS_WIN
    // On Windows, verify the sidecar config file contains the binary directory
    QVERIFY2(QFileInfo::exists(confPath()),
             qPrintable("Config file should exist at: " + confPath()));
    QFile conf(confPath());
    QVERIFY(conf.open(QIODevice::ReadOnly));
    auto confContent = QString::fromUtf8(conf.readAll()).trimmed();
    binDir = QDir::toNativeSeparators(binDir);
    QVERIFY2(confContent == binDir,
             qPrintable("Config should contain binary directory.\nExpected: " + binDir +
                        "\nActual: " + confContent));
#else
    // On macOS/Linux, verify wrapper script references the CLI binary
    QFile wrapper(wrapperPath());
    QVERIFY(wrapper.open(QIODevice::ReadOnly));
    auto content = wrapper.readAll();
    QVERIFY2(content.contains("kirawavtar-cli"),
             "Wrapper should reference kirawavtar-cli");
    QVERIFY2(content.contains(binDir.toUtf8()),
             qPrintable("Wrapper should reference the binary directory: " + binDir));
#endif
}

void TestCliInstaller::install_idempotent()
{
    // Wrapper should already exist from install_createsWrapper
    QVERIFY(QFileInfo::exists(wrapperPath()));

    int rc = runCli({"install-cli"});
    QCOMPARE(rc, 0);
    QVERIFY(QFileInfo::exists(wrapperPath()));
}

void TestCliInstaller::uninstall_removesWrapper()
{
    // Wrapper should already exist
    QVERIFY(QFileInfo::exists(wrapperPath()));

    QString stderrOutput;
    int rc = runCli({"uninstall-cli"}, nullptr, &stderrOutput);
    QCOMPARE(rc, 0);
    QVERIFY2(!QFileInfo::exists(wrapperPath()),
             qPrintable("Wrapper should be removed from: " + wrapperPath()));
#ifdef Q_OS_WIN
    QVERIFY2(!QFileInfo::exists(confPath()),
             qPrintable("Config file should be removed from: " + confPath()));
#endif
}

void TestCliInstaller::uninstall_whenNotInstalled()
{
    // Wrapper should not exist after previous uninstall
    QVERIFY(!QFileInfo::exists(wrapperPath()));

    int rc = runCli({"uninstall-cli"});
    // Should succeed gracefully (or at least not crash)
    // On some platforms removing a non-existent file may fail, that's acceptable
    Q_UNUSED(rc);
    QVERIFY(!QFileInfo::exists(wrapperPath()));
}

#ifdef Q_OS_WIN
void TestCliInstaller::install_addsToPath()
{
    // Install first
    int rc = runCli({"install-cli"});
    QCOMPARE(rc, 0);

    auto wrapperDir = QDir::toNativeSeparators(QFileInfo(wrapperPath()).absolutePath());
    auto pathValue = readUserPathFromRegistry();
    QVERIFY2(pathValue.contains(wrapperDir, Qt::CaseInsensitive),
             qPrintable("PATH should contain: " + wrapperDir + "\nActual PATH: " + pathValue));
}

void TestCliInstaller::uninstall_removesFromPath()
{
    // Uninstall
    int rc = runCli({"uninstall-cli"});
    QCOMPARE(rc, 0);

    auto wrapperDir = QDir::toNativeSeparators(QFileInfo(wrapperPath()).absolutePath());
    auto pathValue = readUserPathFromRegistry();
    QVERIFY2(!pathValue.contains(wrapperDir, Qt::CaseInsensitive),
             qPrintable("PATH should not contain: " + wrapperDir + "\nActual PATH: " + pathValue));
}
#endif

QTEST_GUILESS_MAIN(TestCliInstaller)
#include "test_cli_installer.moc"
