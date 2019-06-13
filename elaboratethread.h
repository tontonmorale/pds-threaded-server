#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
#include "esp.h"
#include <QSharedPointer>

class MyServer;

class ElaborateThread : public QObject
{
    Q_OBJECT

private:
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    int totClients;
    QMap<QString, Person> *peopleMap;
    int currMinute;
    int connectedClients;
    QMap<QString, Esp> *espMap;
    QPointF maxEspCoords;
    void calculateDevicesPosition();
    QList<QPointF> *devicesCoords;
    void manageCurrentMinute();
    void manageLastMinute();
    MyServer* server;

public:
    ElaborateThread();
    ElaborateThread(MyServer* server, QMap<QString, QSharedPointer<Packet>> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    int connectedClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute,
                    QMap<QString, Esp> *espMap,
                    QPointF maxEspCoords,
                    QList<QPointF> *devicesCoords);
    void updatePacketsSet(Person &p, QString shortKey);
    void signalsConnection(QThread *thread);

public slots:
    void work();

signals:
    void finished();
    void log(QString message);
    void timeSlotEndSig();
    void ready();
    void drawMapSig();
};

#endif // ELABORATETHREAD_H
