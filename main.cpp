#include "mainwindow.h"
#include <QApplication>
#include <myserver.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;


    uint curr_timestamp = QDateTime::currentDateTime().toTime_t();
    QString endtime = QString::number(curr_timestamp);

    auto runtimeMap = new QMap<QString, int>();
    runtimeMap->insert("19/03/22_17:52", 4);
    runtimeMap->insert("19/03/22_17:57", 10);

    w.setChartDataSlot(runtimeMap);

    w.show();
//    w.serverInit();
//    w.drawRuntimeChartSlot(new QMap<QString, int>());

    return a.exec();
}
