#include "elaboratethread.h"

#include <QMap>

#define MAX_MINUTES 5

ElaborateThread::ElaborateThread(QMap<QString, QSharedPointer<Packet>> *packetsMap,
                                 QMap<QString, int> *packetsDetectionMap,
                                 int totClients,
                                 QMap<QString, Person> *peopleMap,
                                 int currMinute):
    packetsMap(packetsMap),
    packetsDetectionMap(packetsDetectionMap),
    totClients(totClients),
    peopleMap(peopleMap),
    currMinute(currMinute)
{

}

void ElaborateThread::work() {

    manageCurrentMinute();

    if(currMinute >= MAX_MINUTES)
        manageLastMinute();

}

void ElaborateThread::manageCurrentMinute(){
    QMap<QString, int>::iterator i;
    for(i=packetsDetectionMap->begin(); i!=packetsDetectionMap->end(); i++){
        // se dispositivo rilevato da tutti i client
        if(i.value() >= totClients){

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

//---------------------------------------------
//if last minute
QList<QPointF> points;

{

    //calcolo posizione dispositivi solo se almeno 3 client connessi
    if(MAX_CLIENTS>=3)
    {
        QPointF pA, pB, pC;
        Esp espA, espB, espC;
        espA = (*espList)[0];
        espB = (*espList)[1];
        espC = (*espList)[2];
        pA = espA.getPoint();
        pB = espB.getPoint();
        pC = espC.getPoint();

        //calculate positions of people in area
        QMap<QString, Person>::iterator pm;
        for(pm=this->peopleMap.begin(); pm!=this->peopleMap.end(); pm++){
            qDebug().noquote() << "Mac: " + pm.key();
            qDebug().noquote() << " con i seguenti pacchetti:\n";

            QSet<QSharedPointer<Packet>> ps = pm.value().getPacketsSet();

            double d1, d2, d3;

            //corrispondenza tra un certo esp e la sua potenza
            for (QSet<QSharedPointer<Packet>>::iterator i = ps.begin(); i != ps.end(); i++) {
                if ((*i)->getEsp().compare(espA.getName())==0){
                    d1 = dbToMeters((*i)->getSignal());
//                                d1 = calculateDistance((*i)->getSignal());
                    if((*i)->getMac().compare("30:74:96:94:e3:2d")==0)
                        qDebug().noquote() << QString::number(d1) + " " + QString::number((*i)->getSignal()) + "\n";
                }
                else if ((*i)->getEsp().compare(espB.getName())==0){
                    d2 = dbToMeters((*i)->getSignal());
//                                d2 = calculateDistance((*i)->getSignal());
                    if((*i)->getMac().compare("30:74:96:94:e3:2d")==0)
                        qDebug().noquote() << QString::number(d2) + " " + QString::number((*i)->getSignal()) + "\n";
                }
                else if ((*i)->getEsp().compare(espC.getName())==0){
                    d3 = dbToMeters((*i)->getSignal());
//                                d3 = calculateDistance((*i)->getSignal());
                    if((*i)->getMac().compare("30:74:96:94:e3:2d")==0)
                        qDebug().noquote() << QString::number(d3) + " " + QString::number((*i)->getSignal()) + "\n";
                }
                qDebug().noquote() << (*i)->getHash() + "\n";

            }


            Point res = trilateration(d1, d2, d3, pA, pB, pC);
            QPointF point = QPointF(res.getX(), res.getY());
            if ((point.x()>=0 && point.y()>=0) && (point.x()<=max.x() && point.y()<=max.y()))
                    points.append(point);
            }
        }

    //conta le persone nell'area e disegna grafico
    this->currTimeSlot ++;
    QPointF point = QPointF(this->currTimeSlot * TIME_SLOT, peopleMap.size());
    this->peopleCounter.append(point);
    this->window->setWidget("time", peopleCounter, max);
    //disegna persone nella mappa
    this->window->setWidget("map", points, max);
    this->window->show();

    if(this->currTimeSlot>=12){
        this->currTimeSlot = 0;
        this->peopleCounter.clear();
        QPointF point = QPointF(0, 0);
        this->peopleCounter.append(point);
    }

    //delete previous packets
    this->peopleMap.clear();

    this->currMinute = 0;
    //send start to clients
    startToClients();
    //...todo
    //...todo
    //...todo
}

this->currMinute++;
this->packetsMap.clear();
this->areaPacketsMap.clear();
conn->flush();
//---------------------------------------------

void ElaborateThread::updatePacketsSet(Person &p, QString shortKey){
    QMap<QString, QSharedPointer<Packet>>::iterator itLow, i;
    int n;
    itLow = packetsMap->lowerBound(shortKey);

    p.clearPacketsSet();

    for(i=itLow, n=0; n<totClients; i++, n++)
        p.insertPacket(i.value());

    //check if set contains n=MAX_CLIENTS packets
//    if(packetsSet.size()!=MAX_CLIENTS)
//        continue;
}
