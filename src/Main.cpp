#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("KiraTools");
    a.setApplicationName("KiraWAVTar");
    MainWindow w;
    w.show();
    return a.exec();
}
