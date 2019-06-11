#ifndef DBTHREAD_H
#define DBTHREAD_H

#include "person.h"
#include <QtSql>

class MyServer;

using namespace std;


class DBThread : public QObject
{
    Q_OBJECT

public:
    DBThread();
    DBThread(MyServer* server);
    bool initialized;
    void GetTimestampsFromDB(QMap<QString, int> *peopleCounterMap, QString begintime, QString endtime);
    void GetLPSFromDB(QString begintime, QString endtime);
    void signalsConnection(QThread *thread);
    bool dbConnect();
    void dbDisconnect();
    bool isDbOpen();

public slots:
    void sendSlot(QMap<QString, Person> *peopleMap, int size);
    void run();

signals:
    void finishedSig();
    void fatalErrorSig(QString errorMsg);
    void logSig(QString logMsg);
    void drawRuntimeChartSig();
    void sendFinishedSig();

private:    

    int size;
    QSqlDatabase db;
    QMap<QString, Person> *peopleMap;
    QList<QPointF> drawOldContMap(QMap<QString, int> *oldCountMap);
    QString begintime, endtime;
    MyServer* server;
    QMap<QString, int> *peopleCounterMap;
};




#endif // DBTHREAD_H
