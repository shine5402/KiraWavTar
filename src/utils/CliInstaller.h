#ifndef CLIINSTALLER_H
#define CLIINSTALLER_H

#include <QString>

class QWidget;

namespace utils {

class CliInstaller
{
public:
    static bool isInstalled();
    static bool install(QWidget *parent);
    static bool uninstall(QWidget *parent);
    static QString wrapperPath();

private:
    static QString cliBinaryPath();
};

} // namespace utils

#endif // CLIINSTALLER_H
