#ifndef WAVCOMBINE_H
#define WAVCOMBINE_H

#include <QObject>

//Because we are just writing static functions, so namespace may be a better choice than class.
namespace WAVCombine
{
    QStringList getAbsoluteWAVFileNamesUnder(QString rootDirName, bool recursive = false);
    void doWork(QString rootDirName, bool recursive, QString saveFileName);
};

#endif // WAVCOMBINE_H
