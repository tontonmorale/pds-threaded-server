#ifndef PERSON_H
#define PERSON_H

#include <QString>
#include <QSharedPointer>
#include <QSet>
#include <QPoint>
#include "packet.h"


class Person
{
public:
    Person();
    Person(QString mac);
    int getMinCount();
    void setMinCount(int minutesCount);
    void flushPacketsSet();
    void setPacketsSet(QSet<QSharedPointer<Packet>> packetsSet);
    QSet<QSharedPointer<Packet>> getPacketsSet();
    void insertPacket(QSharedPointer<Packet> p);
    void clearPacketsSet();

private:
    QString mac;
    QSet<QSharedPointer<Packet>> packetsSet;
    int minutesCount;
    QPointF point;
};

#endif // PERSON_H
