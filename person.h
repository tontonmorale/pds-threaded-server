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
    void addPosition(QPointF pos);
    QList<QPointF> getPositionsList();
    void setAvgPosition(QPointF);
    QPointF getAvgPosition();

private:
    QString mac;
    QPointF avgPosition;
    QList<QPointF> positionsList;
};

#endif // PERSON_H
