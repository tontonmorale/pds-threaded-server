#ifndef LISTENEROBJ_H
#define LISTENEROBJ_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"
#include "esp.h"

class MyServer;

using namespace std;

class ListenerThread : public QObject
{
    Q_OBJECT

public:
    ListenerThread();
    ListenerThread(MyServer *server, qintptr socketDescriptor,
                QMutex* mutex,
                QMap<QString, QSharedPointer<Packet>> *packetsMap,
                QMap<QString, int> *packetsDetectionMap,
                QMap<QString, Esp> *espMap);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();
    void closeConnection();
    void newPacket(QString line);
    void signalsConnection(QThread *thread);
    ~ListenerThread();

public slots:
    void work();
    void readFromClient();
    void sendStart();

signals:
    void ready();
    void finished();
    void log(QString message);
    void endPackets();


private:
    QMutex* mutex;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    shared_ptr<QMap<QString, Esp>> espMap;
    MyServer *server;
};

#endif // LISTENEROBJ_H
