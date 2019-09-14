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
    ListenerThread(int &totClients);
    ListenerThread(MyServer *server, qintptr socketDescriptor,
                QMutex* mutex,
                QMap<QString, Packet> *packetsMap,
                QMap<QString, int> *packetsDetectionMap,
                QMap<QString, Esp> *espMap,
                double maxSignal,
                int& totClients,
                int* currMinute);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();

    void newPacket(QString line);
    void signalsConnection(QThread *thread);
    QString getId();
    bool getEndPacketSent();
    ~ListenerThread();
    bool endPacketSent;

public slots:
    void work();
    void readFromClient();
    void sendStart(int currMinute);
    void closeConnection(QString id);
    void beforeClosingSlot();

signals:
    void ready(ListenerThread *);
    void finished(QString);
    void log(QString message);
    void endPackets();
    void beforeDestructionSig();


private:
    QString id;
    QMutex* mutex;
    QMap<QString, Packet> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    shared_ptr<QMap<QString, Esp>> espMap;
    MyServer *server;
    QTimer *disconnectionTimer;
    double maxSignal;
    int& totClients;
    bool firstStart;
    QString tag;
    QThread* thread;
    int* currMinute;
};

#endif // LISTENEROBJ_H
