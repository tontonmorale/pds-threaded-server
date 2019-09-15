#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>
#include <utility.h>
#include "math.h"

double norm(QPointF p){
    double x = p.x(), y=p.y();
    return pow(pow(x,2)+pow(y,2) , 0.5);
}

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
