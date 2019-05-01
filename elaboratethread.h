#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
#include "esp.h"
#include "myserver.h"
#include <QSharedPointer>

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

public:
    ElaborateThread();
    ElaborateThread(QMap<QString, QSharedPointer<Packet>> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    int connectedClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute,
                    QMap<QString, Esp> *espMap,
                    QPointF maxEspCoords,
                    QList<QPointF> *devicesCoords);
    void updatePacketsSet(Person &p, QString shortKey);
    void signalsConnection(QThread *thread, MyServer *server);

public slots:
    void work();

signals:
    void finished();
    void log(QString message);
    void timeSlotEnd();
    void ready();
};

#endif // ELABORATETHREAD_H
