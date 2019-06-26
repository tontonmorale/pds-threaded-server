#include "person.h"
#include "math.h"

Person::Person(){

}

Person::Person(QString mac){
    this->mac = mac;
    this->minutesCount = 1;
    this->point = QPointF(NAN, NAN);
}

int Person::getMinCount(){
    return this->minutesCount;
}

void Person::setMinCount(int minutesCount){
    this->minutesCount = minutesCount;
}

void Person::setPacketsList(QList<Packet> packetsList){
    this->packetsList = packetsList;
}

void Person::insertPacket(Packet p){
    this->packetsList.append(p);
}

QList<Packet> Person::getPacketsList(){
    return this->packetsList;
}

void Person::clearPacketsSet(){
    this->packetsList.clear();
}
