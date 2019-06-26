#ifndef PERSON_H
#define PERSON_H

#include <QString>
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
    void setPacketsList(QList<Packet> packetsList);
    QList<Packet> getPacketsList();
    void insertPacket(Packet p);
    void clearPacketsSet();

private:
    QString mac;
    QList<Packet> packetsList;
    int minutesCount;
    QPointF point;
};

#endif // PERSON_H
