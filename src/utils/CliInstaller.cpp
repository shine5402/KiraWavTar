#include "CliInstaller.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace utils {

QString CliInstaller::cliBinaryPath()
{
    return QCoreApplication::applicationDirPath() + "/kirawavtar-cli"
#ifdef Q_OS_WIN
           + ".exe"
#endif
        ;
}

QString CliInstaller::wrapperPath()
{
#ifdef Q_OS_MACOS
    return "/usr/local/bin/kirawavtar-cli";
#elif defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
           "/Programs/KiraWavTar/bin/kirawavtar-cli.cmd";
#else
    return QDir::homePath() + "/.local/bin/kirawavtar-cli";
#endif
}

bool CliInstaller::isInstalled()
{
    return QFileInfo::exists(wrapperPath());
}

// Run a QProcess without blocking the GUI event loop.
// Returns the exit code, or -1 if the process failed to start.
static int runProcessNonBlocking(QProcess &proc)
{
    QEventLoop loop;
    QObject::connect(&proc, &QProcess::finished, &loop, &QEventLoop::quit);
    QObject::connect(&proc, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    if (proc.state() == QProcess::NotRunning)
        return -1;
    loop.exec();
    return proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : -1;
}

#ifdef Q_OS_MACOS

// Run a shell command with admin privileges via osascript.
// Returns true on success, false on failure or user cancellation.
static bool runWithAdminPrivileges(const QString &shellCmd, QWidget *parent, const QString &errorContext)
{
    // Escape for AppleScript double-quoted string
    QString escaped = shellCmd;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");

    QProcess proc;
    proc.start("osascript",
               {"-e", QString("do shell script \"%1\" with administrator privileges").arg(escaped)});
    int exitCode = runProcessNonBlocking(proc);

    if (exitCode != 0) {
        // AppleScript "User canceled" is error -128; osascript writes it to stderr as "(-128)"
        auto stderrOutput = proc.readAllStandardError();
        if (!stderrOutput.contains("(-128)")) {
            QMessageBox::critical(parent, {},
                                  QObject::tr("Failed to %1:\n%2").arg(errorContext, stderrOutput));
        }
        return false;
    }
    return true;
}

bool CliInstaller::install(QWidget *parent)
{
    auto binPath = cliBinaryPath();
    if (!QFileInfo::exists(binPath)) {
        QMessageBox::critical(parent, {},
                              QObject::tr("CLI binary not found at %1.\n"
                                          "Make sure kirawavtar-cli is installed inside the app bundle.")
                                  .arg(binPath));
        return false;
    }

    auto appDir = QCoreApplication::applicationDirPath();
    auto frameworksDir = QFileInfo(appDir).dir().filePath("Frameworks");

    QString script = QString("#!/bin/bash\n"
                             "APP_DIR=\"%1\"\n"
                             "DYLD_FRAMEWORK_PATH=\"%2\" \\\n"
                             "DYLD_LIBRARY_PATH=\"%2\" \\\n"
                             "exec \"$APP_DIR/kirawavtar-cli\" \"$@\"\n")
                         .arg(appDir, frameworksDir);

    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();

    // Try writing directly first (may work if user owns /usr/local/bin)
    QDir().mkpath(wrapperDir);
    QFile wrapperFile(wrapper);
    if (wrapperFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        wrapperFile.write(script.toUtf8());
        wrapperFile.close();
        wrapperFile.setPermissions(wrapperFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
                                   QFileDevice::ExeOther);
        return true;
    }

    // Need admin privileges — write to a temp file first, then copy with elevated privileges.
    // This avoids embedding script content in a shell string (injection risk).
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open()) {
        QMessageBox::critical(parent, {},
                              QObject::tr("Failed to create temporary file for CLI wrapper."));
        return false;
    }
    tmpFile.write(script.toUtf8());
    tmpFile.close();

    auto tmpPath = tmpFile.fileName();
    QString shellCmd = QString("mkdir -p '%1' && cp '%2' '%3' && chmod +x '%3' && rm -f '%2'")
                           .arg(wrapperDir, tmpPath, wrapper);

    if (!runWithAdminPrivileges(shellCmd, parent, QObject::tr("install CLI wrapper"))) {
        QFile::remove(tmpPath);
        return false;
    }

    return true;
}

bool CliInstaller::uninstall(QWidget *parent)
{
    auto wrapper = wrapperPath();

    if (QFile::remove(wrapper))
        return true;

    // Need admin privileges — wrapperPath() is hardcoded, no injection risk
    QString shellCmd = QString("rm -f '%1'").arg(wrapper);
    return runWithAdminPrivileges(shellCmd, parent, QObject::tr("uninstall CLI wrapper"));
}

#elif defined(Q_OS_WIN)

// Parse the user PATH value from `reg query HKCU\Environment /v Path` output.
// Returns the PATH string, or a null QString if parsing fails.
static QString readUserPathFromRegistry()
{
    QProcess proc;
    proc.start("reg", {"query", "HKCU\\Environment", "/v", "Path"});
    proc.waitForFinished(5000);

    if (proc.exitCode() != 0)
        return {};

    // reg query output format: "    Path    REG_EXPAND_SZ    <value>"
    // Use regex to robustly extract the value after the type field.
    QString output = proc.readAllStandardOutput();
    QRegularExpression re(R"(Path\s+REG_(?:EXPAND_)?SZ\s+(.*))");
    auto match = re.match(output);
    if (match.hasMatch())
        return match.captured(1).trimmed();

    return {};
}

bool CliInstaller::install(QWidget *parent)
{
    auto binPath = cliBinaryPath();
    if (!QFileInfo::exists(binPath)) {
        QMessageBox::critical(
            parent, {},
            QObject::tr("CLI binary not found at %1.").arg(binPath));
        return false;
    }

    auto appDir = QCoreApplication::applicationDirPath();
    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();

    QDir().mkpath(wrapperDir);

    // Write .cmd wrapper
    QString script = QString("@echo off\r\n"
                             "set \"KIRAWAVTAR_DIR=%1\"\r\n"
                             "set \"PATH=%%KIRAWAVTAR_DIR%%;%%PATH%%\"\r\n"
                             "\"%%KIRAWAVTAR_DIR%%\\kirawavtar-cli.exe\" %%*\r\n")
                         .arg(QDir::toNativeSeparators(appDir));

    QFile wrapperFile(wrapper);
    if (!wrapperFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, {},
                              QObject::tr("Failed to write CLI wrapper to %1.").arg(wrapper));
        return false;
    }
    wrapperFile.write(script.toLocal8Bit());
    wrapperFile.close();

    // Add to user PATH via registry
    auto nativeWrapperDir = QDir::toNativeSeparators(wrapperDir);
    auto pathValue = readUserPathFromRegistry();

    if (pathValue.isNull()) {
        // No user PATH exists or registry query failed — create one with just our entry
        // This is safe: we're not overwriting anything, just creating a new user PATH
        pathValue = nativeWrapperDir;
    } else if (!pathValue.contains(nativeWrapperDir, Qt::CaseInsensitive)) {
        // Append our directory to existing PATH
        if (!pathValue.isEmpty())
            pathValue += ";";
        pathValue += nativeWrapperDir;
    } else {
        // Already in PATH
        return true;
    }

    QProcess regProc;
    regProc.start("reg", {"add", "HKCU\\Environment", "/v", "Path", "/t", "REG_EXPAND_SZ", "/d", pathValue, "/f"});
    regProc.waitForFinished(5000);

    if (regProc.exitCode() != 0) {
        QMessageBox::warning(parent, {},
                             QObject::tr("CLI wrapper was created but failed to add to PATH.\n"
                                         "You may need to add %1 to your PATH manually.")
                                 .arg(nativeWrapperDir));
        return true; // wrapper was still created
    }

    // Broadcast WM_SETTINGCHANGE so running shells pick up the change.
    // Use setx to set then immediately delete a dummy variable.
    QProcess::startDetached("cmd", {"/c", "setx", "KIRAWAVTAR_PATH_REFRESH", "1",
                                    "&&", "reg", "delete", "HKCU\\Environment", "/v", "KIRAWAVTAR_PATH_REFRESH", "/f"});

    return true;
}

bool CliInstaller::uninstall(QWidget *parent)
{
    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();
    auto nativeWrapperDir = QDir::toNativeSeparators(wrapperDir);

    QFile::remove(wrapper);

    // Remove from user PATH
    auto pathValue = readUserPathFromRegistry();
    if (!pathValue.isNull() && pathValue.contains(nativeWrapperDir, Qt::CaseInsensitive)) {
        auto entries = pathValue.split(';');
        // Case-insensitive removal
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [&](const QString &entry) {
                                         auto trimmed = entry.trimmed();
                                         if (trimmed.endsWith('\\'))
                                             trimmed.chop(1);
                                         return trimmed.compare(nativeWrapperDir, Qt::CaseInsensitive) == 0;
                                     }),
                      entries.end());
        auto newPath = entries.join(';');

        QProcess regProc;
        regProc.start("reg", {"add", "HKCU\\Environment", "/v", "Path", "/t", "REG_EXPAND_SZ", "/d", newPath, "/f"});
        regProc.waitForFinished(5000);
    }

    // Clean up directory only if truly empty (rmdir fails on non-empty dirs, unlike removeRecursively)
    QDir().rmdir(wrapperDir);

    return true;
}

#else // Linux

bool CliInstaller::install(QWidget *parent)
{
    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();
    QDir().mkpath(wrapperDir);

    bool isFlatpak = QFileInfo::exists("/.flatpak-info") || !qEnvironmentVariable("FLATPAK_ID").isEmpty();

    if (isFlatpak) {
        QString script = "#!/bin/bash\n"
                         "exec flatpak run --command=kirawavtar-cli top.shine5402.KiraWavTar \"$@\"\n";

        QFile wrapperFile(wrapper);
        if (!wrapperFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(parent, {},
                                  QObject::tr("Failed to write CLI wrapper to %1.").arg(wrapper));
            return false;
        }
        wrapperFile.write(script.toUtf8());
        wrapperFile.close();
        wrapperFile.setPermissions(wrapperFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
                                   QFileDevice::ExeOther);
    } else {
        auto binPath = cliBinaryPath();
        if (!QFileInfo::exists(binPath)) {
            QMessageBox::critical(parent, {},
                                  QObject::tr("CLI binary not found at %1.").arg(binPath));
            return false;
        }

        QFile::remove(wrapper);

        if (!QFile::link(binPath, wrapper)) {
            QMessageBox::critical(parent, {},
                                  QObject::tr("Failed to create symlink at %1.").arg(wrapper));
            return false;
        }
    }

    return true;
}

bool CliInstaller::uninstall(QWidget *parent)
{
    auto wrapper = wrapperPath();
    if (!QFile::remove(wrapper)) {
        QMessageBox::critical(parent, {},
                              QObject::tr("Failed to remove %1.").arg(wrapper));
        return false;
    }
    return true;
}

#endif

} // namespace utils
