#ifndef KIRA_FILESYSTEM_H
#define KIRA_FILESYSTEM_H

#include <QString>

QStringList getAbsoluteAudioFileNamesUnder(QString rootDirName, bool recursive = false);

#endif // KIRA_FILESYSTEM_H
