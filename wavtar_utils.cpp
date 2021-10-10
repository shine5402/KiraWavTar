#include "wavtar_utils.h"
#include <QDir>
#include <QStack>
#include <exception>
#include <QStackedWidget>
#include <QTime>

namespace wavtar_utils {
    QStringList getAbsoluteWAVFileNamesUnder(QString rootDirName, bool recursive){
        auto timeStart = QTime::currentTime();

        QStack<QString> dirNameStack;
        dirNameStack.append(rootDirName);

        QStringList result;
        while (!dirNameStack.isEmpty()){
            auto currentDirName = dirNameStack.pop();
            QDir currentDir{currentDirName};

            auto wavEntryList = currentDir.entryList({"*.wav"}, QDir::Files | QDir::NoDotAndDotDot);
            for (const auto& entry : std::as_const(wavEntryList)){
               result.append(currentDir.filePath(entry));
            }
            if (!recursive)
                break;

            auto dirsToPush = currentDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const auto& i : std::as_const(dirsToPush)){
                dirNameStack.push(currentDir.filePath(i));
            }
        }

        auto timeEnd = QTime::currentTime();
        auto duration = timeStart.msecsTo(timeEnd);
        qDebug("[getAbsoluteWAVFileNamesUnder] duration=%d", duration);
        return result;
    }

    QString getDescFileNameFrom(const QString& WAVFileName){
        auto descFileName = WAVFileName;
        descFileName.remove(QRegExp("(\\.wav)$"));
        descFileName.append(".kirawavtar-desc.json");
        return descFileName;
    }

}
