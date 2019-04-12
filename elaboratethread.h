#ifndef ELABORATETHREAD_H
#define ELABORATETHREAD_H


#include "packet.h"
#include "person.h"
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

public:
    ElaborateThread(QMap<QString, QSharedPointer<Packet>> *packetsMap,
                    QMap<QString, int> *packetsDetectionMap,
                    int totClients,
                    QMap<QString, Person> *peopleMap,
                    int currMinute);
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
