#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication::setStyle("Fusion");
    QApplication a(argc, argv);
    a.setOrganizationName("KiraTools");
    a.setApplicationName("KiraWAVTar");
    MainWindow w;
    w.show();
    return a.exec();
}
