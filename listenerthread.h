#ifndef LISTENEROBJ_H
#define LISTENEROBJ_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include <QTimer>
#include "packet.h"
#include "esp.h"
#include "math.h"

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
                QMap<QString, Esp> *espMap,
                double maxSignal);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();
    void closeConnection();
    void newPacket(QString line);
    void signalsConnection(QThread *thread);
    QString getId();
    ~ListenerThread();

public slots:
    void work();
    void readFromClient();
    void sendStart();

signals:
    void ready();
    void finished(ListenerThread*);
    void log(QString message);
    void endPackets();
    void addThreadSignal(ListenerThread*);


private:
    QString id;
    QMutex* mutex;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    shared_ptr<QMap<QString, Esp>> espMap;
    MyServer *server;
    QTimer* disconnectionTimer;
    double maxSignal;
};

#endif // LISTENEROBJ_H
