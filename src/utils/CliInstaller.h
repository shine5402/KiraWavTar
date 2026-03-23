#ifndef CLIINSTALLER_H
#define CLIINSTALLER_H

#include <QString>

namespace utils {

class CliInstaller
{
public:
    static bool isInstalled();
    /// Install the CLI wrapper. Returns true on success.
    /// On failure, writes a diagnostic to errorMessage (if non-null).
    static bool install(QString *errorMessage = nullptr);
    /// Uninstall the CLI wrapper. Returns true on success.
    /// On failure, writes a diagnostic to errorMessage (if non-null).
    static bool uninstall(QString *errorMessage = nullptr);
    static QString wrapperPath();
    static QString cliBinaryPath();
};

} // namespace utils

#endif // CLIINSTALLER_H
