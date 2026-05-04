#include "CliInstaller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#endif

namespace utils {

static void setError(QString *errorMessage, const QString &msg)
{
    if (errorMessage)
        *errorMessage = msg;
}

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
#ifdef Q_OS_WIN
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
           "/Programs/KiraWavTar/bin/kirawavtar-cli.exe";
#else
    return QDir::homePath() + "/.local/bin/kirawavtar-cli";
#endif
}

bool CliInstaller::isInstalled()
{
    return QFileInfo::exists(wrapperPath());
}

#if defined(Q_OS_WIN)

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

static QString launcherSourcePath()
{
    return QCoreApplication::applicationDirPath() + "/kirawavtar-cli-launcher.exe";
}

static QString wrapperConfPath()
{
    return QFileInfo(CliInstaller::wrapperPath()).absolutePath() + "/kirawavtar-cli.conf";
}

bool CliInstaller::install(QString *errorMessage)
{
    auto binPath = cliBinaryPath();
    if (!QFileInfo::exists(binPath)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","CLI binary not found at %1.").arg(binPath));
        return false;
    }

    auto launcherSrc = launcherSourcePath();
    if (!QFileInfo::exists(launcherSrc)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","CLI launcher not found at %1.").arg(launcherSrc));
        return false;
    }

    auto appDir = QCoreApplication::applicationDirPath();
    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();

    QDir().mkpath(wrapperDir);

    // Copy launcher exe to wrapper directory
    QFile::remove(wrapper); // remove old copy if exists
    if (!QFile::copy(launcherSrc, wrapper)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","Failed to copy CLI launcher to %1.").arg(wrapper));
        return false;
    }

    // Write sidecar config with path to real binary directory
    auto confPath = wrapperConfPath();
    QFile confFile(confPath);
    if (!confFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","Failed to write config file %1.").arg(confPath));
        QFile::remove(wrapper);
        return false;
    }
    confFile.write(QDir::toNativeSeparators(appDir).toUtf8());
    confFile.close();

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
        setError(errorMessage,
                 QCoreApplication::translate("CliInstaller","CLI wrapper was created but failed to add to PATH.\n"
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

bool CliInstaller::uninstall(QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();
    auto nativeWrapperDir = QDir::toNativeSeparators(wrapperDir);

    QFile::remove(wrapper);
    QFile::remove(wrapperConfPath());

    // Remove legacy .cmd wrapper from older installations
    QFile::remove(QFileInfo(wrapper).absolutePath() + "/kirawavtar-cli.cmd");

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

#else // macOS and Linux

static bool writeShellWrapper(const QString &wrapper, const QString &script, QString *errorMessage)
{
    QFile wrapperFile(wrapper);
    if (!wrapperFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","Failed to write CLI wrapper to %1.").arg(wrapper));
        return false;
    }
    wrapperFile.write(script.toUtf8());
    wrapperFile.close();
    wrapperFile.setPermissions(wrapperFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
                               QFileDevice::ExeOther);
    return true;
}

bool CliInstaller::install(QString *errorMessage)
{
    auto binPath = cliBinaryPath();
    if (!QFileInfo::exists(binPath)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","CLI binary not found at %1.").arg(binPath));
        return false;
    }

    auto wrapper = wrapperPath();
    auto wrapperDir = QFileInfo(wrapper).absolutePath();
    QDir().mkpath(wrapperDir);
    QFile::remove(wrapper);

#ifdef Q_OS_MACOS
    auto appDir = QCoreApplication::applicationDirPath();
    auto frameworksDir = QFileInfo(appDir).dir().filePath("Frameworks");

    QString script = QString("#!/bin/bash\n"
                             "APP_DIR=\"%1\"\n"
                             "DYLD_FRAMEWORK_PATH=\"%2\" \\\n"
                             "DYLD_LIBRARY_PATH=\"%2\" \\\n"
                             "exec \"$APP_DIR/kirawavtar-cli\" \"$@\"\n")
                         .arg(appDir, frameworksDir);
    return writeShellWrapper(wrapper, script, errorMessage);
#else // Linux
    bool isFlatpak = QFileInfo::exists("/.flatpak-info") || !qEnvironmentVariable("FLATPAK_ID").isEmpty();

    if (isFlatpak) {
        QString script = "#!/bin/bash\n"
                         "exec flatpak run --command=kirawavtar-cli top.shine5402.KiraWavTar \"$@\"\n";
        return writeShellWrapper(wrapper, script, errorMessage);
    }

    if (!QFile::link(binPath, wrapper)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","Failed to create symlink at %1.").arg(wrapper));
        return false;
    }
    return true;
#endif
}

bool CliInstaller::uninstall(QString *errorMessage)
{
    auto wrapper = wrapperPath();
    if (!QFile::remove(wrapper)) {
        setError(errorMessage, QCoreApplication::translate("CliInstaller","Failed to remove %1.").arg(wrapper));
        return false;
    }
    return true;
}

#endif

} // namespace utils
