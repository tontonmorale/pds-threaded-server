#include "mainwindow.h"
#include "myserver.h"
#include <fstream>
#include <memory>
using namespace std;

#define ESP_FILE_PATH "C:/Users/tonio/Desktop/ServerPds/esp.txt"

MyServer::MyServer(QObject *parent): QTcpServer (parent), connectedClients(0), totClients(0) {
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
    espMap = new QMap<QString, Esp>();
    mutex = new QMutex();
    objList = new QList<ListenerObj*>();
    packetsMap = new QMap<QString, QSharedPointer<Packet>>();
    packetsDetectionMap = new QMap<QString, int>;
    peopleMap = new QMap<QString, Person>;
}

void MyServer::init(){
    confFromFile();
}

void MyServer::incomingConnection(qintptr socketDescriptor){
    QThread *thread = new QThread();
    ListenerObj *obj = new ListenerObj(socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap);

    obj->moveToThread(thread);

    connect(thread, SIGNAL(started()), obj, SLOT(work()));

    connect(obj, SIGNAL(ready()), this, SLOT(startToClients()));
    connect(this, SIGNAL(start2Clients()), obj, SLOT(sendStart()));
    connect(obj, &ListenerObj::log, this, &MyServer::emitLog);
    connect(obj, SIGNAL(endPackets()), this, SLOT(createElaborateThread()));

    connect(obj, SIGNAL(finished()), thread, SLOT(quit()));
    connect(obj, SIGNAL(finished()), obj, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    objList->append(obj);

    thread->start();

}

void MyServer::createElaborateThread(){
    QMutex mutex;
    mutex.lock();
    endPkClients ++;
    if(endPkClients<totClients){
        return;
    }

    // tutti i client hanno mandato i pacchetti
    endPkClients = 0;
    //insert in peopleMap if packet received by all clients
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
                peopleMap[mac] = p;
            }
            else{
                //check if mac already considered in current minute
                int count = (*peopleMap)[mac].getMinCount();
                if(count < this->currMinute){
                    Person p = this->peopleMap[mac];
                    this->peopleMap[mac].setMinCount(count+1);
                    updatePacketsSet(this->peopleMap[mac], shortKey);
                }
            }
        }
    }

}

void MyServer::updatePacketsSet(Person &p, QString shortKey){
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

void MyServer::emitLog(QString message){
    emit log(message);
}

void MyServer::startToClients(){
    // manda start se tutti i client sono connessi
    connectedClients++; // --- da rivedere per poterla richiamare anche ai 5 minuti successivi
    if(connectedClients==totClients
//            && connectedClients>=3
            ){
        qDebug() << "Start to clients\n";
        endPkClients = 0;
        emit log("\n[server] sending start...\n");
        emit start2Clients();
    }
}

void MyServer::confFromFile(){
    ifstream inputFile;
    int i, nClients;
    string id, mac, x_str, y_str;
    QString qmac;
    double x_double, y_double;

    for(i=0; i<3; i++){
        inputFile.open(ESP_FILE_PATH);
        if (inputFile) {
            break;
        }
    }
    if(i>=3){
        // --- emettere segnale di errore
        qDebug() << "Errore apertura file";
        emit error("Impossibile aprire il file degli esp");
        return;
    }

    inputFile >> nClients; // --- nClients è da sostituire con totClients ---
    for(i=0; i< nClients; i++){ // --- nClients è da sostituire con totClients ---
        inputFile >> id >> mac >> x_str >> y_str;
        x_double = stod(x_str.c_str());
        y_double = stod(y_str.c_str());
        QPointF point = QPointF(x_double, y_double);
        qmac = QString::fromStdString(mac);
        const Esp esp = Esp(QString::fromStdString(id), qmac, point);
        espMap->insert(qmac, esp);
    }

    inputFile.close();
}



