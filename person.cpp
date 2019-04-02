#include "person.h"

#define INF std::numeric_limits<double>::max()

Person::Person(){

}

Person::Person(QString mac){
    this->mac = mac;
    this->packetsSet = packetsSet;
    this->minutesCount = 1;
    this->point = QPointF(INF, INF);
}

int Person::getMinCount(){
    return this->minutesCount;
}

void Person::setMinCount(int minutesCount){
    this->minutesCount = minutesCount;
}

void Person::setPacketsSet(QSet<QSharedPointer<Packet>> packetsSet){
    this->packetsSet = packetsSet;
}

void Person::insertPacket(QSharedPointer<Packet> p){
    this->packetsSet.insert(p);
}

QSet<QSharedPointer<Packet>> Person::getPacketsSet(){
    return this->packetsSet;
}

void Person::clearPacketsSet(){
    this->packetsSet.clear();
}
