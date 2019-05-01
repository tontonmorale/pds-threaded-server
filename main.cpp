#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;


    uint curr_timestamp = QDateTime::currentDateTime().toTime_t();
    QString endtime = QString::number(curr_timestamp);

    w.show();
    w.serverInit();

    return a.exec();
}
