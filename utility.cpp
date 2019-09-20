#include "utility.h"

#define TXPOWER -50
#define N 2.0

Utility::Utility()
{

}

double Utility::dbToMeters(int signal){
    double d;

    d = pow(10, (TXPOWER-signal)/(10*N));
    return d;
}

double Utility::metersToDb(double meters){
    return TXPOWER - 10 * N * log10( meters );
}

double Utility::norm(QPointF p){
    double x = p.x(), y=p.y();
    return pow(pow(x,2)+pow(y,2) , 0.5);
}

/**
 * @brief Utility::canTriangulate -> sostituisce l'uso dell'if nelle altre classi per trovare meglio i punti in cui viene utilizzato
 * @param connectedClients
 * @return
 */
bool Utility::canTriangulate(int connectedClients){
    return connectedClients >= 3;
}

QPointF Utility::trilateration(double r1, double r2, double r3, QPointF pos1, QPointF pos2, QPointF pos3) {
    QPointF resultPose;

    //unit vector in a direction from point1 to point 2
    double p2p1Distance = pow(pow(pos2.x()-pos1.x(),2) + pow(pos2.y() -  pos1.y(),2),0.5);
    QPointF cos_sin = {(pos2.x()-pos1.x())/p2p1Distance, (pos2.y()-pos1.y())/p2p1Distance};
    QPointF p3Byp1 = {pos3.x()-pos1.x(),pos3.y()-pos1.y()};

    //signed magnitude of the x component
    double i = cos_sin.x() * p3Byp1.x() + cos_sin.y() * p3Byp1.y();

    //the unit vector in the y direction.
    QPointF aux2 = { pos3.x()-pos1.x()-i*cos_sin.x(), pos3.y()-pos1.y()-i*cos_sin.y()};
    QPointF ey = { aux2.x() / norm(aux2), aux2.y() / norm(aux2) };

    //the signed magnitude of the y component
    double j = ey.x() * p3Byp1.x() + ey.y() * p3Byp1.y();

    //coordinates
    double x = (pow(r1,2) - pow(r2,2) + pow(p2p1Distance,2))/ (2 * p2p1Distance);
    double y = (pow(r1,2) - pow(r3,2) + pow(i,2) + pow(j,2))/(2*j) - i*x/j;

    //result coordinates
    double finalX = pos1.x()+ x*cos_sin.x() + y*ey.x();
    double finalY = pos1.y()+ x*cos_sin.y() + y*ey.y();
    resultPose.setX(finalX);
    resultPose.setY(finalY);
    return resultPose;
}
