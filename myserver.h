#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <listenerobj.h>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"
#include "esp.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer(QObject *parent = 0);
    void confFromFile();
    void init();
    void SendToDB();

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
    QList<ListenerObj*> *objList;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};



#endif // MYSERVER_H
