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
    DBThread(QMap<QString, Person> *peopleMap, int size, bool initialized, QString begintime, QString endtime, QList<QPointF> *peopleCounter);
    bool initialized;
    void GetTimestampsFromDB();
    void GetLPSFromDB(QString begintime, QString endtime);
    void signalsConnection(QThread *thread, MyServer *server, QString slotName);

public slots:
    void send();
    bool init();

signals:
    void finished();

private:
    QSqlDatabase db;
    bool dbConnect();
    int size;
    QMap<QString, Person> *peopleMap;
    QList<QPointF> drawOldContMap(QMap<QString, int> *oldCountMap);
    QString begintime, endtime;
    QList<QPointF> *peopleCounter;
};




#endif // DBTHREAD_H
