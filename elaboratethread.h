#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
#include "esp.h"
#include <QMutex>

class MyServer;

class ElaborateThread : public QObject
{
    Q_OBJECT

private:
    QMutex *mutex;
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
    QString tag;

public:
    ElaborateThread();
    ElaborateThread(MyServer* server, QMap<QString, Packet> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    QMutex* mutex,
                    int connectedClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute,
                    QMap<QString, Esp> *espMap,
                    QPointF maxEspCoords,
                    QList<QPointF> *devicesCoords);

    QList<Packet> getPacketsList(QString shortKey);
    void signalsConnection(QThread *thread);
    ~ElaborateThread();

public slots:
    void work();

signals:
    void log(QString message, QString color);
    void elabFinishedSig();
//    void ready();
    void drawMapSig();
    void finish();
    void setMinuteSig(int);
};

#endif // ELABORATETHREAD_H
