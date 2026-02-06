#include "WavTarUtils.h"
#include <QDir>
#include <QStack>
#include <exception>
#include <QStackedWidget>

namespace wavtar_utils {
    QString getDescFileNameFrom(const QString& WAVFileName){
        auto fileInfo = QFileInfo(WAVFileName);
        return fileInfo.absoluteDir().absoluteFilePath(fileInfo.completeBaseName() + ".kirawavtar-desc.json");
    }

}
