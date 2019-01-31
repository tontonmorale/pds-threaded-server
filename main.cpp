#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if(!w.showWindow)
        return -1;

    w.show();
    return a.exec();
}
