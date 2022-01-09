#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication::setStyle("Fusion");
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icon/appIcon"));
    MainWindow w;
    w.show();
    return a.exec();
}
