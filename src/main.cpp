#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <kira/darkmode.h>

int main(int argc, char *argv[])
{
    QApplication::setStyle("Fusion");
    QApplication a(argc, argv);
    a.setOrganizationName("KiraTools");
    a.setApplicationName("KiraWAVTar");
    DarkMode::setCurrentPaletteToApp();
    MainWindow w;
    w.show();
    return a.exec();
}
