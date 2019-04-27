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
    DBThread(QMap<QString, Person> *peopleMap, bool initialized);
    bool initialized;
    void GetTimestampsFromDB(QString begintime, QString endtime, QList<QPointF> *peopleCounter);
    void GetLPSFromDB(QString begintime, QString endtime);
    void signalsConnection(QThread *thread, MyServer *server);

public slots:
    void send();
    bool init();

signals:
    void finished();

private:
    QSqlDatabase db;
    bool dbConnect();
    QMap<QString, Person> *peopleMap;
    QList<QPointF> drawOldContMap(QMap<QString, int> *oldCountMap);
};




#endif // DBTHREAD_H
