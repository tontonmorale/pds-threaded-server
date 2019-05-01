#include "utility.h"

#define TXPOWER -42

Utility::Utility()
{

}

double Utility::dbToMeters(int signal){
    double d;
    double n = 1;

    d = pow(10, (TXPOWER-signal)/(10*n));
    return d;
}

double Utility::norm(QPointF p){
    double x = p.x(), y=p.y();
    return pow(pow(x,2)+pow(y,2) , 0.5);
}

QPointF Utility::trilateration(double r1, double r2, double r3, QPointF pos1, QPointF pos2, QPointF pos3) {
    QPointF resultPose;

    //unit vector in a direction from point1 to point 2
    double p2p1Distance = pow(pow(pos2.x()-pos1.x(),2) + pow(pos2.y() -  pos1.y(),2),0.5);
    QPointF ex = {(pos2.x()-pos1.x())/p2p1Distance, (pos2.y()-pos1.y())/p2p1Distance};
    QPointF aux = {pos3.x()-pos1.x(),pos3.y()-pos1.y()};

    //signed magnitude of the x component
    double i = ex.x() * aux.x() + ex.y() * aux.y();

    //the unit vector in the y direction.
    QPointF aux2 = { pos3.x()-pos1.x()-i*ex.x(), pos3.y()-pos1.y()-i*ex.y()};
    QPointF ey = { aux2.x() / norm(aux2), aux2.y() / norm(aux2) };

    //the signed magnitude of the y component
    double j = ey.x() * aux.x() + ey.y() * aux.y();

    //coordinates
    double x = (pow(r1,2) - pow(r2,2) + pow(p2p1Distance,2))/ (2 * p2p1Distance);
    double y = (pow(r1,2) - pow(r3,2) + pow(i,2) + pow(j,2))/(2*j) - i*x/j;

    //result coordinates
    double finalX = pos1.x()+ x*ex.x() + y*ey.y();
    double finalY = pos1.y()+ x*ex.y() + y*ey.y();
    resultPose.setX(finalX);
    resultPose.setY(finalY);
    return resultPose;
}
