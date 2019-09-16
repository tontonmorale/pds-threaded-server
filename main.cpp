#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>
#include <utility.h>
#include "math.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;


//    uint curr_timestamp = QDateTime::currentDateTime().toTime_t();
//    QString endtime = QString::number(curr_timestamp);

    w.serverInit();
    w.show();


    return a.exec();
}
