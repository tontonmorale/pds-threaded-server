#ifndef DBTHREAD_H
#define DBTHREAD_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include <QtSql>
#include "packet.h"
#include "esp.h"
#include "person.h"

using namespace std;


class DBThread : public QObject
{
    Q_OBJECT

public:
    DBThread();
    DBThread(QMap<QString, Person> *peopleMap, bool initialized);
    bool initialized;
    void GetTimestampsFromDB(QString begintime, QString endtime);
    void GetLPSFromDB(QString begintime, QString endtime);

public slots:
    void send();   
    bool init();

signals:
    void finished();

private:
    QSqlDatabase db;
    bool dbConnect();
    QMap<QString, Person> *peopleMap;
};




#endif // DBTHREAD_H
