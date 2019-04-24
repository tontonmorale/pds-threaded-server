#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
#include "esp.h"
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
    QList<Esp> *espList;
    QPointF maxEspCoords;
    QList<QPointF> calculateDevicesPosition();

public:
    ElaborateThread();
    ElaborateThread(QMap<QString, QSharedPointer<Packet>> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    int connectedClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute,
                    QMap<QString, Esp> *espMap,
                    QPointF maxEspCoords);
    void updatePacketsSet(Person &p, QString shortKey);

protected:
    void manageCurrentMinute();
    void manageLastMinute();

public slots:
    void work();

signals:
    void finished();
    void log(QString message);
};

#endif // ELABORATETHREAD_H
