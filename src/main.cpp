#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("KiraTools");
    a.setApplicationName("KiraWAVTar");

#if defined(Q_OS_MAC)
    const QString iconPath = QCoreApplication::applicationDirPath() + "/../Resources/KiraWavTar.icns";
    if (QFileInfo::exists(iconPath)) {
        a.setWindowIcon(QIcon(iconPath));
    }
#endif

    MainWindow w;
    w.show();
    return a.exec();
}
