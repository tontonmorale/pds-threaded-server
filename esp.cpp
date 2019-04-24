#include "esp.h"

Esp::Esp(){

}

Esp::Esp(QString id, QString mac, QPointF pos){
    this->id = id;
    this->mac = mac;
    this->position = pos;
}

QString Esp::getId(){
    return this->id;
}

QPointF Esp::getPosition(){
    return this->position;
}
