#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "listenerthread.h"
#include "packet.h"
#include "person.h"
#include "esp.h"
#include "dbthread.h"
#include "elaboratethread.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer(QObject *parent = nullptr);
    void confFromFile();
    void init();
    void SendToDB();
    QList<QPointF> *DrawOldCountMap(QString begintime, QString endtime);
    void Connects(QString slot);
    ~MyServer() override;

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
    void clearPeopleMap();

private:
    QMutex* mutex;
//    void onClientConnection(qintptr socketDescriptor);
//    QPlainTextEdit *log;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QMap<QString, Esp> *espMap;
    QMap<QString, Person> *peopleMap;
    QMap<QString, int> *oldCountMap;
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
