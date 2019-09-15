#include "person.h"
#include "math.h"

Person::Person(){

}

Person::Person(QString mac): mac(mac){}

void Person::addPosition(QPointF pos){
    positionsList.append(pos);
}

QList<QPointF> Person::getPositionsList(){
    return positionsList;
}

void Person::setAvgPosition(QPointF pos){
    avgPosition = pos;
}

QPointF Person::getAvgPosition() {
    return avgPosition;
}
