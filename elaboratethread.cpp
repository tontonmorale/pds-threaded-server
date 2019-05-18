#include "elaboratethread.h"
#include "myserver.h"
#include "utility.h"

#include <QMap>

#define MAX_MINUTES 5

ElaborateThread::ElaborateThread() {}

ElaborateThread::ElaborateThread(MyServer* server, QMap<QString, QSharedPointer<Packet>> *packetsMap,
                                 QMap<QString, int> *packetsDetectionMap,
                                 int connectedClients,
                                 QMap<QString, Person> *peopleMap,
                                 int currMinute,
                                 QMap<QString, Esp> *espMap,
                                 QPointF maxEspCoords,
                                 QList<QPointF> *devicesCoords):
    packetsMap(packetsMap),
    packetsDetectionMap(packetsDetectionMap),
    peopleMap(peopleMap),
    currMinute(currMinute),
    connectedClients(connectedClients),
    espMap(espMap),
    maxEspCoords(maxEspCoords),
    devicesCoords(devicesCoords),
    server(server)
{

}

void ElaborateThread::work() {
    // minuto attuale
    manageCurrentMinute();

    // ultimo minuto
    if(currMinute >= MAX_MINUTES){
        currMinute = 0;
        emit ready(); // manda start alle schede per nuovo timeslot
        manageLastMinute();
        emit timeSlotEnd(); // manda dati time slot corrente al thread che si occupa del db e alla gui
    }
}

void ElaborateThread::signalsConnection(QThread *thread){

    connect(thread, SIGNAL(started()), this, SLOT(work()));

    connect(this, &ElaborateThread::log, server, &MyServer::emitLog);
    connect(this, &ElaborateThread::timeSlotEnd, server, &MyServer::dataForDb);
    connect(this, &ElaborateThread::ready, server, &MyServer::startToClients);

    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

void ElaborateThread::manageCurrentMinute(){
    QMap<QString, int>::iterator i;
    for(i=packetsDetectionMap->begin(); i!=packetsDetectionMap->end(); i++){
        // se dispositivo rilevato da tutti i client
        if(i.value() >= connectedClients){

            QString shortKey = i.key();
            QString mac = shortKey.split('-').at(1);

            //if mac is not in peopleMap
            if(peopleMap->find(mac) == peopleMap->end()){

                Person p = Person(mac);
                updatePacketsSet(p, shortKey);
                //insert new person in peopleMap
                (*peopleMap)[mac] = p;
            }
            else{
                //check if mac already considered in current minute
                int count = (*peopleMap)[mac].getMinCount();
                if(count < this->currMinute){
                    Person p = (*peopleMap)[mac];
                    (*peopleMap)[mac].setMinCount(count+1);
                    updatePacketsSet((*peopleMap)[mac], shortKey);
                }
            }
        }
    }
}

void ElaborateThread::manageLastMinute() {

    //calcolo posizione dispositivi solo se almeno 3 client connessi
    if(connectedClients < 3){
        emit log("Non ci sono abbastanza dispositivi connessi\n");
        return;
    }

    //calcola lista di posizioni dei device nell'area e la salva in devicesCoords
    calculateDevicesPosition();
}

void ElaborateThread::calculateDevicesPosition(){
    QPointF posA, posB, posC;
    Esp espA, espB, espC;
    QMap<QString, Esp>::iterator it;

    it = espMap->begin();
    espA = it.value();
    it++;
    espB = it.value();
    it++;
    espC = it.value();
    posA = espA.getPosition();
    posB = espB.getPosition();
    posC = espC.getPosition();

    //calcola posizioni dispositivi nell'area
    QMap<QString, Person>::iterator person;
    for(person = peopleMap->begin(); person != peopleMap->end(); person++){
        qDebug().noquote() << "Mac: " + person.key();
        qDebug().noquote() << " con i seguenti pacchetti:\n";

        QSet<QSharedPointer<Packet>> set = person.value().getPacketsSet();

        double d1, d2, d3;

        //corrispondenza tra un certo esp e la sua potenza
        for (QSet<QSharedPointer<Packet>>::iterator p = set.begin(); p != set.end(); p++) {
            if ((*p)->getEspId().compare(espA.getId())==0){
                d1 = Utility::dbToMeters((*p)->getSignal());
                if((*p)->getMac().compare("30:74:96:94:e3:2d")==0)
                    qDebug().noquote() << QString::number(d1) + " " + QString::number((*p)->getSignal()) + "\n";
            }
            else if ((*p)->getEspId().compare(espB.getId())==0){
                d2 = Utility::dbToMeters((*p)->getSignal());
                if((*p)->getMac().compare("30:74:96:94:e3:2d")==0)
                    qDebug().noquote() << QString::number(d2) + " " + QString::number((*p)->getSignal()) + "\n";
            }
            else if ((*p)->getEspId().compare(espC.getId())==0){
                d3 = Utility::dbToMeters((*p)->getSignal());
                if((*p)->getMac().compare("30:74:96:94:e3:2d")==0)
                    qDebug().noquote() << QString::number(d3) + " " + QString::number((*p)->getSignal()) + "\n";
            }
            qDebug().noquote() << (*p)->getHash() + "\n";

        }

        QPointF pos = Utility::trilateration(d1, d2, d3, posA, posB, posC);
        //se punto nell'area delimitata dagli esp
        if ((pos.x()>=0 && pos.y()>=0) && (pos.x()<=maxEspCoords.x() && pos.y()<=maxEspCoords.y()))
            devicesCoords->append(pos);
    }
}

void ElaborateThread::updatePacketsSet(Person &p, QString shortKey){
    QMap<QString, QSharedPointer<Packet>>::iterator itLow, i;
    int n;
    itLow = packetsMap->lowerBound(shortKey);

    p.clearPacketsSet();

    for(i=itLow, n=0; n<connectedClients; i++, n++)
        p.insertPacket(i.value());

    //check if set contains n=MAX_CLIENTS packets
//    if(packetsSet.size()!=MAX_CLIENTS)
//        continue;
}
