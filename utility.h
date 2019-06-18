#ifndef UTILITY_H
#define UTILITY_H

#include "math.h"
#include "QPointF"

class Utility
{
public:
    Utility();
    double static dbToMeters(int signal);
    QPointF static trilateration(double r1, double r2, double r3, QPointF esp1, QPointF esp2, QPointF esp3);
    double static norm(QPointF p);
    bool static canTriangulate(int connectedClients);
};

#endif // UTILITY_H
