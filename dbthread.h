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
    void GetLPSFromDB(QString begintime, QString endtime);
    void signalsConnection(QThread *thread);
    bool dbConnect();
    void dbDisconnect();
    bool isDbOpen();
    void getChartDataFromDb(QString begintime, QString endtime);
    QDateTime calculateTimestamp();
    ~DBThread();

public slots:
    void sendChartDataToDbSlot(QMap<QString, Person> peopleMap);
    void run();
    void getMinDateForLPSTATSSlot();

signals:
    void finished();
    void fatalErrorSig(QString errorMsg);
    void logSig(QString logMsg, QString color);
    void drawChartSig(QMap<QString, int> chartDataToDrawMap);
    void dbConnectedSig();
    void LPStatsWindowCreationSig(QString, QString);
    void LPStatsSig(QMap<QString, QList<QString>>);

private:
    QSqlDatabase db;
    QString begintime, endtime;
    MyServer* server;
    QString tag;
};




#endif // DBTHREAD_H
