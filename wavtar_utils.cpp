#include "wavtar_utils.h"
#include <QDir>
#include <QStack>
#include <exception>
#include <QStackedWidget>
#include <QTime>
#include <QtConcurrent>

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
        qDebug("[getAbsoluteWAVFileNamesUnder] traditional_duration=%d", duration);

        auto traditionalCount  = result.count();
        result.clear();
        QMutex resultMutex;
        timeStart = QTime::currentTime();
        QStringList pendingDirs;

        pendingDirs.append(rootDirName);

        while (!pendingDirs.isEmpty()){
            auto dirNext = QtConcurrent::blockingMappedReduced<QStringList>(pendingDirs, std::function([&result, &resultMutex](const QString& dirName) mutable -> QStringList{
                QDir currentDir{dirName};
                auto wavEntryList = currentDir.entryList({"*.wav"}, QDir::Files | QDir::NoDotAndDotDot);
                auto wavFileNames = QtConcurrent::blockingMapped(wavEntryList, std::function([currentDir](const QString& entry)->QString{return currentDir.filePath(entry);}));
                QMutexLocker resultLocker(&resultMutex);
                result.append(wavFileNames);
                resultLocker.unlock();
                return QtConcurrent::blockingMapped(currentDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot), std::function([currentDir](const QString& dirName) -> QString{return currentDir.filePath(dirName);}));
            }), [](QStringList& dirNext, const QStringList& dirsNextEach){
                dirNext.append(dirsNextEach);
            });
            if (!recursive)
                break;
            pendingDirs = dirNext;
        }
        timeEnd = QTime::currentTime();
        qDebug("[getAbsoluteWAVFileNamesUnder] concurrent_duration=%d", duration);
        Q_ASSERT(traditionalCount == result.count());
        return result;
    }

    QString getDescFileNameFrom(const QString& WAVFileName){
        auto fileInfo = QFileInfo(WAVFileName);
        return fileInfo.absoluteDir().absoluteFilePath(fileInfo.completeBaseName() + ".kirawavtar-desc.json");
    }

}
