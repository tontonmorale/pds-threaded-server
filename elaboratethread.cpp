#include "elaboratethread.h"
#include "myserver.h"
#include "utility.h"

#include <QMap>

#define MAX_MINUTES 5

ElaborateThread::ElaborateThread() {}

ElaborateThread::ElaborateThread(MyServer* server, QMap<QString, Packet> *packetsMap,
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

    qDebug() << "[elaborate thread] gestito minuto corrente";

    // ultimo minuto
    if(currMinute >= MAX_MINUTES){
        currMinute = 0;
//        emit ready(); // manda start alle schede per nuovo timeslot
        manageLastMinute();
        emit elabFinishedSig(); // manda dati time slot corrente al thread che si occupa del db e alla gui
        qDebug() << "[elaborate thread] gestito ultimo minuto";

    }
    qDebug() << "[elaborate thread] in teoria sto per morire";

    emit finished();
}

/**
 * @brief ElaborateThread::signalsConnection
 * collega segnali-slot tra questo thread e il server
 * @param thread
 */
void ElaborateThread::signalsConnection(QThread *thread){

    connect(thread, SIGNAL(started()), this, SLOT(work()));

    connect(this, &ElaborateThread::log, server, &MyServer::emitLogSlot);
    connect(this, &ElaborateThread::elabFinishedSig, server, &MyServer::onChartDataReadySlot);
//    connect(this, &ElaborateThread::ready, server, &MyServer::startToClientsSlot);

    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

//    connect(this, &ElaborateThread::drawMapSig, server, &MyServer::drawMapSlot);
}

/**
 * @brief ElaborateThread::manageCurrentMinute
 * aggiunge in peopleMap le nuove persone rilevate da tutti gli esp nel minuto corrente
 */
void ElaborateThread::manageCurrentMinute(){
    QMap<QString, int>::iterator i;
    for(i=packetsDetectionMap->begin(); i!=packetsDetectionMap->end(); i++){

        if(i.value() >= 3){
            // dispositivo rilevato da almeno 3 client

            QString shortKey = i.key(); // shortKey = "pktHash-mac"
            QString mac = shortKey.split('-').at(1);

//            if(mac.compare("30:74:96:94:e3:2d")==0 || mac.compare("94:65:2d:41:f7:8c")==0 ){ // andrà tolto

                auto person = peopleMap->find(mac);
                if(person == peopleMap->end()){
                    // nuovo device

                    Person p = Person(mac);                    
                    person = peopleMap->insert(mac, p);
                }

                // cerco pacchetti nella packetMap associati a quello corrente di packetsDetectioMap
                QList<Packet> packetsList = getPacketsList(shortKey);

                // ricavo posizioni esp che hanno rilevato i pacchetti
                QPointF posA, posB, posC;
                Esp espA, espB, espC;
                double d1, d2, d3; // distanze tra device e i vari esp

                auto packet = packetsList.begin();
                for(auto esp : *espMap){
                    if(esp.getId().compare(packet->getEspId())==0){
                        espA = esp;
                        posA = esp.getPosition();
                        d1 = Utility::dbToMeters(packet->getSignal());
                        if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                            qDebug().noquote() << QString::number(d1) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }
                packet++;
                for(auto esp : *espMap){
                    if(esp.getId().compare(packet->getEspId())==0){
                        espB = esp;
                        posB = esp.getPosition();
                        d2 = Utility::dbToMeters(packet->getSignal());
                        if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                            qDebug().noquote() << QString::number(d2) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }
                packet++;
                for(auto esp : *espMap){
                    if(esp.getId().compare(packet->getEspId())==0){
                        espC = esp;
                        posC = esp.getPosition();
                        d3 = Utility::dbToMeters(packet->getSignal());
                        if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                            qDebug().noquote() << QString::number(d3) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }

                QPointF pos = Utility::trilateration(d1, d2, d3, posA, posB, posC);
                if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                    qDebug() << "posx: " + QString::number(pos.x()) + " posy: " + QString::number(pos.y());
                person->addPosition(pos);

//            }
        }
    }
    packetsMap->clear();
    packetsDetectionMap->clear();
    // TODO: si può fare la media delle trilaterazioni per ogni persona
}

/**
 * @brief ElaborateThread::manageLastMinute
 * chiama funzione calcolo posizionidei device nell'area
 */
void ElaborateThread::manageLastMinute() {

    //calcola media triangolazioni
    calculateAvgPosition();
}

/**
 * @brief ElaborateThread::calculateDevicesPosition
 * medie delle trilaterazioni
 */
void ElaborateThread::calculateAvgPosition(){

    // media trilaterazioni
    QMap<QString, Person>::iterator person;
    for(person=peopleMap->begin(); person!=peopleMap->end(); person++){
        QList<QPointF> positionsList = person->getPositionsList();
        QPointF avg;
        for(auto pos : positionsList){
            avg += pos;
        }
        avg /= positionsList.length();
        person->setAvgPosition(avg);
    }

//    if ((pos.x()>=0 && pos.y()>=0) && (pos.x()<=maxEspCoords.x() && pos.y()<=maxEspCoords.y())){
//        //device all'interno dell'area delimitata dagli esp => aggiungilo a devicesCoords
//        devicesCoords->append(pos);
//    }
//    else{
//        QMap<QString, Person>::iterator temp = person+1;
//        //togli persone dall'elenco
//        peopleMap->erase(person);
//        person = temp;

//        if(peopleMap->size()==0)
//            break;
//    }
}

/**
 * @brief ElaborateThread::updatePacketsSet
 * aggiorna il set di pacchetti (è sempre lo stesso pacchetto, stessa shortkey, ma rilevato da esp diversi) usato come rappresentativo di un device
 * per capire dove si trova
 * @param p: device considerato
 * @param shortKey: identifica un pacchetto (quindi un device)
 */
QList<Packet> ElaborateThread::getPacketsList(QString shortKey){
    QMap<QString, Packet>::iterator itLow, i;
    int n;
    QList<Packet> packetsList;

    // iteratore che punta al primo dei pacchetti con key = shortKey inviati nel minuto corrente dal device
    itLow = packetsMap->lowerBound(shortKey);

    // pacchetto con key=shortKey ripetuto almeno 3 volte
    for(i=itLow, n=0; n<3; i++, n++)
        packetsList.append(i.value());

    return packetsList;
}
