#include "Utils.h"

#include <QDir>
#include <QStack>
#include <QStackedWidget>
#include <exception>

namespace utils {
QString getDescFileNameFrom(const QString &WAVFileName)
{
    auto fileInfo = QFileInfo(WAVFileName);
    return fileInfo.absoluteDir().absoluteFilePath(fileInfo.completeBaseName() + ".kirawavtar-desc.json");
}

} // namespace utils
