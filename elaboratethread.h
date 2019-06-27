#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
#include "esp.h"

class MyServer;

class ElaborateThread : public QObject
{
    Q_OBJECT

private:
    QMap<QString, Packet> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    int totClients;
    QMap<QString, Person> *peopleMap;
    int currMinute;
    int connectedClients;
    QMap<QString, Esp> *espMap;
    QPointF maxEspCoords;
    void calculateAvgPosition();
    QList<QPointF> *devicesCoords;
    void manageCurrentMinute();
    void manageLastMinute();
    MyServer* server;

public:
    ElaborateThread();
    ElaborateThread(MyServer* server, QMap<QString, Packet> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    int connectedClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute,
                    QMap<QString, Esp> *espMap,
                    QPointF maxEspCoords,
                    QList<QPointF> *devicesCoords);

    QList<Packet> getPacketsList(QString shortKey);
    void signalsConnection(QThread *thread);

public slots:
    void work();

signals:
    void finished();
    void log(QString message);
    void elabFinishedSig();
    void ready();
    void drawMapSig();
};

#endif // ELABORATETHREAD_H
