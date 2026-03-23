#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>

class TestCliLauncher : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void forwardsVersionCommand();
    void forwardsExitCode();
    void forwardsArguments();
    void failsWithMissingConfig();
    void failsWithEmptyConfig();
    void failsWithBadPath();

private:
    QString cliBinaryPath() const { return QString::fromUtf8(CLI_BINARY_PATH); }
    QString launcherBinaryPath() const { return QString::fromUtf8(LAUNCHER_BINARY_PATH); }

    // Set up a temp directory with the launcher and a config file, return the launcher path in temp.
    // Returns empty string on failure.
    QString setupLauncher(QTemporaryDir &tmpDir, const QString &confContent) const;

    int runLauncher(const QString &launcherPath, const QStringList &args,
                    QString *stdoutOutput = nullptr, QString *stderrOutput = nullptr) const;
};

void TestCliLauncher::initTestCase()
{
    QVERIFY2(QFileInfo::exists(cliBinaryPath()),
             qPrintable("CLI binary not found: " + cliBinaryPath()));
    QVERIFY2(QFileInfo::exists(launcherBinaryPath()),
             qPrintable("Launcher binary not found: " + launcherBinaryPath()));
}

QString TestCliLauncher::setupLauncher(QTemporaryDir &tmpDir, const QString &confContent) const
{
    if (!tmpDir.isValid())
        return {};

    auto launcherDest = tmpDir.path() + "/kirawavtar-cli.exe";
    QFile::copy(launcherBinaryPath(), launcherDest);

    if (!confContent.isNull()) {
        QFile conf(tmpDir.path() + "/kirawavtar-cli.conf");
        if (!conf.open(QIODevice::WriteOnly | QIODevice::Text))
            return {};
        conf.write(confContent.toUtf8());
    }

    return launcherDest;
}

int TestCliLauncher::runLauncher(const QString &launcherPath, const QStringList &args,
                                 QString *stdoutOutput, QString *stderrOutput) const
{
    QProcess proc;
    proc.start(launcherPath, args);
    proc.waitForFinished(30000);
    if (stdoutOutput)
        *stdoutOutput = QString::fromUtf8(proc.readAllStandardOutput());
    if (stderrOutput)
        *stderrOutput = QString::fromUtf8(proc.readAllStandardError());
    if (proc.exitStatus() != QProcess::NormalExit)
        return -1;
    return proc.exitCode();
}

void TestCliLauncher::forwardsVersionCommand()
{
    QTemporaryDir tmpDir;
    auto binDir = QDir::toNativeSeparators(QFileInfo(cliBinaryPath()).absolutePath());
    auto launcher = setupLauncher(tmpDir, binDir);
    QVERIFY(!launcher.isEmpty());

    QString stdoutOutput;
    int rc = runLauncher(launcher, {"--version"}, &stdoutOutput);
    QCOMPARE(rc, 0);
    QVERIFY2(stdoutOutput.contains("kirawavtar-cli"),
             qPrintable("Expected version output, got: " + stdoutOutput));
}

void TestCliLauncher::forwardsExitCode()
{
    QTemporaryDir tmpDir;
    auto binDir = QDir::toNativeSeparators(QFileInfo(cliBinaryPath()).absolutePath());
    auto launcher = setupLauncher(tmpDir, binDir);
    QVERIFY(!launcher.isEmpty());

    // Pass an invalid command to get a non-zero exit code
    QString stderrOutput;
    int rc = runLauncher(launcher, {"--nonexistent-flag"}, nullptr, &stderrOutput);
    QVERIFY2(rc != 0, "Expected non-zero exit code for invalid flag");
}

void TestCliLauncher::forwardsArguments()
{
    QTemporaryDir tmpDir;
    auto binDir = QDir::toNativeSeparators(QFileInfo(cliBinaryPath()).absolutePath());
    auto launcher = setupLauncher(tmpDir, binDir);
    QVERIFY(!launcher.isEmpty());

    // --help should succeed and contain usage info
    QString stdoutOutput;
    int rc = runLauncher(launcher, {"--help"}, &stdoutOutput);
    QCOMPARE(rc, 0);
    QVERIFY2(stdoutOutput.contains("--version"),
             qPrintable("Expected help output to mention --version, got: " + stdoutOutput));
}

void TestCliLauncher::failsWithMissingConfig()
{
    QTemporaryDir tmpDir;
    // Pass null QString so no config file is created
    auto launcher = setupLauncher(tmpDir, QString());
    QVERIFY(!launcher.isEmpty());

    QString stderrOutput;
    int rc = runLauncher(launcher, {"--version"}, nullptr, &stderrOutput);
    QVERIFY2(rc != 0, "Expected failure when config file is missing");
    QVERIFY2(stderrOutput.contains("kirawavtar-cli"),
             qPrintable("Expected error message, got: " + stderrOutput));
}

void TestCliLauncher::failsWithEmptyConfig()
{
    QTemporaryDir tmpDir;
    auto launcher = setupLauncher(tmpDir, "");
    QVERIFY(!launcher.isEmpty());

    QString stderrOutput;
    int rc = runLauncher(launcher, {"--version"}, nullptr, &stderrOutput);
    QVERIFY2(rc != 0, "Expected failure when config file is empty");
}

void TestCliLauncher::failsWithBadPath()
{
    QTemporaryDir tmpDir;
    auto launcher = setupLauncher(tmpDir, "C:\\nonexistent\\path\\that\\does\\not\\exist");
    QVERIFY(!launcher.isEmpty());

    QString stderrOutput;
    int rc = runLauncher(launcher, {"--version"}, nullptr, &stderrOutput);
    QVERIFY2(rc != 0, "Expected failure when config points to nonexistent directory");
}

QTEST_GUILESS_MAIN(TestCliLauncher)
#include "test_cli_launcher.moc"
