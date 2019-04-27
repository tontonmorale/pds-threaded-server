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
#include "myserver.h"

using namespace std;

class ListenerThread : public QObject
{
    Q_OBJECT

public:
    ListenerThread();
    ListenerThread(qintptr socketDescriptor,
                QMutex* mutex,
                QMap<QString, QSharedPointer<Packet>> *packetsMap,
                QMap<QString, int> *packetsDetectionMap,
                QMap<QString, Esp> *espMap);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();
    void closeConnection();
    void newPacket(QString line);
    void signalsConnection(QThread *thread, MyServer *server);
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
};

#endif // LISTENEROBJ_H
