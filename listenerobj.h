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

using namespace std;

class ListenerObj : public QObject
{
    Q_OBJECT

public:
    ListenerObj();
    ListenerObj(qintptr socketDescriptor,
                QMutex* mutex,
                QMap<QString, QSharedPointer<Packet>> *packetsMap,
                QMap<QString, int> *packetsDetectionMap,
                QMap<QString, Esp> *espMap);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();
    void closeConnection();
    void newPacket(QString line);
    ~ListenerObj();

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
