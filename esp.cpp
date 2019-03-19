#include "esp.h"

Esp::Esp(){

}

Esp::Esp(QString id, QString mac, QPointF coord){
    this->id = id;
    this->mac = mac;
    this->coord = coord;
}

QString Esp::getId(){
    return this->id;
}
