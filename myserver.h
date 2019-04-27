#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <listenerthread.h>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"
#include "person.h"
#include "esp.h"
#include "dbthread.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer(QObject *parent = 0);
    void confFromFile();
    void init();
    void SendToDB();
    void DrawOldCountMap(QString begintime, QString endtime);
    void Connects(QString slot);

signals:
    void start2Clients();
    void error(QString message);
    void log(QString message);
    void DBsignal(QMap<QString, Person> *peopleMap);

public slots:
//    void onClientConnection();
    void startToClients();
    void emitLog(QString message);
    void createElaborateThread();
    void dataForDb();
    void readyFromClient();
    ~MyServer() override;

private:
    QMutex* mutex;
//    void onClientConnection(qintptr socketDescriptor);
//    QPlainTextEdit *log;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QMap<QString, Esp> *espMap;
    QMap<QString, Person> *peopleMap;
    int connectedClients;
    int totClients;
    int endPkClients;
    int currMinute;
    QList<ListenerThread*> *listenerThreadList;
    bool DBinitialized;
    QPointF maxEspCoords;
    QPointF setMaxEspCoords(QMap<QString, Esp> *espMap);
    QList<QPointF> *devicesCoords;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};



#endif // MYSERVER_H
