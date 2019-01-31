#include "packet.h"
#include "QString"
using namespace std;

Packet::Packet(){

}

Packet::Packet(QString hash, QString mac, QString timestamp, QString signal, QString esp, QString ssid) {
    this->esp = esp;
    this->hash = hash;
    this->mac = mac;
    this->timestamp = timestamp;
    this->signal = signal.toInt();
    this->ssid = ssid;
}

QString Packet::getHash() {
    return this->hash;
}

QString Packet::getMac() {
    return this->mac;
}

QString Packet::getEsp() {
    return this->esp;
}

int Packet::getSignal() {
    return this->signal;
}

