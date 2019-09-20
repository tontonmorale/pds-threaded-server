#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>
#include <utility.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.serverInit();
    w.show();

//    qDebug().noquote() << QString::number(Utility::dbToMeters(-35));

    return a.exec();
}
