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

/**
 * @brief ElaborateThread::work
 * chiamata all'avvio del thread
 */
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

/**
 * @brief ElaborateThread::signalsConnection
 * collega segnali-slot tra questo thread e il server
 * @param thread
 */
void ElaborateThread::signalsConnection(QThread *thread){

    connect(thread, SIGNAL(started()), this, SLOT(work()));

    connect(this, &ElaborateThread::log, server, &MyServer::emitLog);
    connect(this, &ElaborateThread::timeSlotEnd, server, &MyServer::dataForDb);
    connect(this, &ElaborateThread::ready, server, &MyServer::startToClients);

    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

/**
 * @brief ElaborateThread::manageCurrentMinute
 * aggiunge in peopleMap le nuove persone rilevate da tutti gli esp nel minuto corrente
 */
void ElaborateThread::manageCurrentMinute(){
    QMap<QString, int>::iterator i;
    for(i=packetsDetectionMap->begin(); i!=packetsDetectionMap->end(); i++){

        if(i.value() >= connectedClients){
            // dispositivo rilevato da tutti i client

            QString shortKey = i.key(); // shortKey = "pktHash-mac"
            QString mac = shortKey.split('-').at(1);

            if(peopleMap->find(mac) == peopleMap->end()){
                // device non rilevato nei minuti precedenti

                Person p = Person(mac);
                updatePacketsSet(p, shortKey);
                //insert new person in peopleMap
                (*peopleMap)[mac] = p;
            }
            else{
                // device già rilevato nei minuti precedenti, controlla se già considerato nel minuto corrente
                int count = (*peopleMap)[mac].getMinCount();
                if(count < this->currMinute){
                    // aggiorna i pacchetti del device con quelli rilevati al minuto corrente
                    Person p = (*peopleMap)[mac];
                    (*peopleMap)[mac].setMinCount(count+1);
                    updatePacketsSet((*peopleMap)[mac], shortKey);
                }
            }
        }
    }
}

/**
 * @brief ElaborateThread::manageLastMinute
 * chiama funzione calcolo posizionidei device nell'area
 */
void ElaborateThread::manageLastMinute() {

    //calcolo posizione dispositivi solo se almeno 3 client connessi
    if(connectedClients < 3){
        emit log("Non ci sono abbastanza dispositivi connessi\n");
        return;
    }

    //calcola lista di posizioni dei device nell'area e la salva in devicesCoords
    calculateDevicesPosition();
}

/**
 * @brief ElaborateThread::calculateDevicesPosition
 * calcola posizione dei device nell'area
 */
void ElaborateThread::calculateDevicesPosition(){
    QPointF posA, posB, posC;
    Esp espA, espB, espC;
    QMap<QString, Esp>::iterator it;

    // prende 3 esp dall'elenco
    it = espMap->begin();
    espA = it.value();
    it++;
    espB = it.value();
    it++;
    espC = it.value();
    posA = espA.getPosition(); // coord espA
    posB = espB.getPosition(); // coord espB
    posC = espC.getPosition(); // coord espC

    //calcola posizioni dispositivi nell'area
    QMap<QString, Person>::iterator person;
    for(person = peopleMap->begin(); person != peopleMap->end(); person++){
        qDebug().noquote() << "Mac: " + person.key();
        qDebug().noquote() << " con i seguenti pacchetti:\n";

        // set di pacchetti associati a quel device (stesso pacchetto ma rilevato dai vari esp)
        QSet<QSharedPointer<Packet>> set = person.value().getPacketsSet();

        double d1, d2, d3; // distanze tra device e i vari esp

        // conversione da intensità segnale a metri
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
        if ((pos.x()>=0 && pos.y()>=0) && (pos.x()<=maxEspCoords.x() && pos.y()<=maxEspCoords.y())){
            // device all'interno dell'area delimitata dagli esp => aggiungilo a devicesCoords
            devicesCoords->append(pos);
        }
    }
}

/**
 * @brief ElaborateThread::updatePacketsSet
 * aggiorna il set di pacchetti (è sempre lo stesso pacchetto, stessa shortkey, ma rilevato da esp diversi) usato come rappresentativo di un device
 * per capire dove si trova
 * @param p: device considerato
 * @param shortKey: identifica un pacchetto (quindi un device)
 */
void ElaborateThread::updatePacketsSet(Person &p, QString shortKey){
    QMap<QString, QSharedPointer<Packet>>::iterator itLow, i;
    int n;

    // iteratore che punta al primo dei pacchetti con key = shortKey inviati nel minuto corrente dal device
    itLow = packetsMap->lowerBound(shortKey);

    p.clearPacketsSet();

    // pacchetto con key=shortKey ripetuto n=connecteedClients volte
    for(i=itLow, n=0; n<connectedClients; i++, n++)
        p.insertPacket(i.value());
}
