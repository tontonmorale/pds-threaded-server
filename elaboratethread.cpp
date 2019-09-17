#include "elaboratethread.h"
#include "myserver.h"
#include "utility.h"

#include <QMap>

ElaborateThread::ElaborateThread() {}

ElaborateThread::ElaborateThread(MyServer* server, QMap<QString, Packet> *packetsMap,
                                 QMap<QString, int> *packetsDetectionMap,
                                 QMutex *mutex,
                                 int connectedClients,
                                 QMap<QString, Person> *peopleMap,
                                 int* currMinute,
                                 QMap<QString, Esp> *espMap,
                                 QPointF maxEspCoords,
                                 QList<QPointF> *devicesCoords):
    mutex(mutex),
    packetsMap(packetsMap),
    packetsDetectionMap(packetsDetectionMap),
    peopleMap(peopleMap),
    currMinute(currMinute),
    connectedClients(connectedClients),
    espMap(espMap),
    maxEspCoords(maxEspCoords),
    devicesCoords(devicesCoords),
    server(server),
    tag("ElaborateThread") {
}

/**
 * @brief ElaborateThread::work
 * chiamata all'avvio del thread
 */
void ElaborateThread::work() {
    // minuto attuale
    try {
        mutex->lock();
        manageCurrentMinute();
        (*currMinute)++;
        if(*currMinute<=MAX_MINUTES)
            emit log(tag + ": Current minute = " + QString::number(*currMinute));
        emit setMinuteSig(*currMinute);
        mutex->unlock();
    } catch (exception e) {
        emit log(tag + " : " + e.what());
        mutex->unlock();
        throw e;
    }

    // ultimo minuto
    mutex->lock();
    if(*currMinute >= MAX_MINUTES+1){
        *currMinute = 1;
        emit setMinuteSig(*currMinute);
        emit log(tag + ": Current minute = " + QString::number(*currMinute));
        //        emit ready(); // manda start alle schede per nuovo timeslot
        try {
            manageLastMinute();
        } catch (exception e) {
            emit log(tag + " : " + e.what());
            mutex->unlock();
            throw e;
        }
        emit elabFinishedSig(); // manda dati time slot corrente al thread che si occupa del db e alla gui
    }
    emit finish();
    mutex->unlock();
}

/**
 * @brief ElaborateThread::signalsConnection
 * collega segnali-slot tra questo thread e il server
 * @param thread
 */
void ElaborateThread::signalsConnection(QThread *thread){

    connect(thread, &QThread::started, this, &ElaborateThread::work);

    connect(this, &ElaborateThread::log, server, &MyServer::emitLogSlot);
    connect(this, &ElaborateThread::elabFinishedSig, server, &MyServer::onChartDataReadySlot);

    connect(this, &ElaborateThread::finish, this, &ElaborateThread::deleteLater);
    connect(this, &ElaborateThread::setMinuteSig, server, &MyServer::setMinuteSlot);

    //    connect(this, &ElaborateThread::finish, thread, &QThread::quit);

    //    connect(thread, &QThread::finished, this, &ElaborateThread::deleteLater);
    //    connect(thread, &QThread::quit, thread, &QThread::deleteLater);

    //    connect(this, &ElaborateThread::drawMapSig, server, &MyServer::drawMapSlot);
}

/**
 * @brief ElaborateThread::manageCurrentMinute
 * aggiunge in peopleMap le nuove persone rilevate da tutti gli esp nel minuto corrente
 */
void ElaborateThread::manageCurrentMinute(){

    try {
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
                            qDebug().noquote() << tag << ": " << QString::number(d1) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }
                packet++;
                for(auto esp : *espMap){
                    if(esp.getId().compare(packet->getEspId())==0){
                        espB = esp;
                        posB = esp.getPosition();
                        d2 = Utility::dbToMeters(packet->getSignal());
                        if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                            qDebug().noquote() << tag << ": " << QString::number(d2) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }
                packet++;
                for(auto esp : *espMap){
                    if(esp.getId().compare(packet->getEspId())==0){
                        espC = esp;
                        posC = esp.getPosition();
                        d3 = Utility::dbToMeters(packet->getSignal());
                        if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                            qDebug().noquote() << tag << ": " << QString::number(d3) + " " + QString::number(packet->getSignal()) + "\n";
                    }
                }

                QPointF pos = Utility::trilateration(d1, d2, d3, posA, posB, posC);
                if(packet->getMac().compare("30:74:96:94:e3:2d")==0 || packet->getMac().compare("94:65:2d:41:f7:8c")==0)
                    qDebug() << tag << ": posx: " + QString::number(pos.x()) + " posy: " + QString::number(pos.y());
                person->addPosition(pos);

                //            }
            }
        }
        packetsMap->clear();
        packetsDetectionMap->clear();
        qDebug().noquote() << tag << ": gestito minuto corrente";
    } catch (out_of_range e) {
        packetsMap->clear();
        packetsDetectionMap->clear();
        throw out_of_range("There was some problem with ranges in structures, aborting current minute elaboration.");
    }
    catch (exception e) {
        packetsMap->clear();
        packetsDetectionMap->clear();
        qDebug().noquote() << tag << "problema nella manageCurrentMinute";
        throw e;
    }

}

/**
 * @brief ElaborateThread::manageLastMinute
 * chiama funzione calcolo posizionidei device nell'area
 */
void ElaborateThread::manageLastMinute() {

    //calcola media triangolazioni
    try {
        calculateAvgPosition();
    } catch (out_of_range e) {
        qDebug().noquote() << tag << "problema nella manageLastMinute";
        throw e;
    }
}

/**
 * @brief ElaborateThread::calculateDevicesPosition
 * medie delle trilaterazioni
 */
void ElaborateThread::calculateAvgPosition(){

    try {
        QMap<QString, Person>::iterator person;
        QPointF min = {(-maxEspCoords.x()*2), (-maxEspCoords.y()*2)}, max = {maxEspCoords.x()*2, maxEspCoords.y()*2};
        for(person=peopleMap->begin(); person!=peopleMap->end();){
            QList<QPointF> positionsList = person->getPositionsList();
            QPointF avg = {NAN, NAN};
            for(auto pos : positionsList){
                if ((pos.x()>=min.x() && pos.x()<=max.x()) && (pos.y()>=min.y() && pos.y()<=max.y())){
                    if (isnan(avg.x()) && isnan(avg.y()))
                        avg = {0.0,0.0};
                    avg += pos;
                }
            }
//            emit log(person.key() + " : " + QString::number(avg.x()) + ", " + QString::number(avg.y()));
            qDebug() << person.key() << " : " + QString::number(avg.x()) + ", " + QString::number(avg.y());
            if (!isnan(avg.x()) && !isnan(avg.y())) {
                avg /= positionsList.length();
                person->setAvgPosition(avg);
                //device all'interno dell'area delimitata dagli esp => aggiungilo a devicesCoords
                devicesCoords->append(avg);
                person++;
            }
            else {
                //togli persone dall'elenco
                auto next = peopleMap->erase(person++);
                if (next == peopleMap->end()|| peopleMap->size()==0)
                   break;
            }
        }
        qDebug().noquote() << tag << ": gestito ultimo minuto";
    } catch (out_of_range e) {
        throw out_of_range("Note: there was some problem with ranges in structures calculating the average positions.");
    }
    catch (exception& e) {
        qDebug().noquote() << tag << " problema nella calculateAvgPosition: " << e.what();
        throw e;
    }
    // media trilaterazioni
}

/**
 * @brief ElaborateThread::updatePacketsSet
 * aggiorna il set di pacchetti (è sempre lo stesso pacchetto, stessa shortkey, ma rilevato da esp diversi) usato come rappresentativo di un device
 * per capire dove si trova
 * @param p: device considerato
 * @param shortKey: identifica un pacchetto (quindi un device)
 */
QList<Packet> ElaborateThread::getPacketsList(QString shortKey){
    //qui no try catch perchè è una funzione che viene chiamata dentro manageCurrentMinute
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

ElaborateThread::~ElaborateThread() {
    qDebug() << tag << ": Distruttore ElaborateThread";
}
