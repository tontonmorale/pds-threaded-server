#include "mainwindow.h"
#include "myserver.h"
#include "elaboratethread.h"
#include <fstream>
#include <memory>
using namespace std;

#define ESP_FILE_PATH "C:/Users/raffy/Downloads/PDS_progetto/pds-threaded-server/esp.txt"

MyServer::MyServer(QObject *parent): QTcpServer (parent), connectedClients(0), totClients(1) {
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
    espMap = new QMap<QString, Esp>();
    mutex = new QMutex();
    listenerThreadList = new QList<ListenerThread*>();
    packetsMap = new QMap<QString, QSharedPointer<Packet>>();
    packetsDetectionMap = new QMap<QString, int>;
    peopleMap = new QMap<QString, Person>;
    DBinitialized = false;
}

QPointF MyServer::setMaxEspCoords(QMap<QString, Esp> *espMap) {
    double x_max = 0, y_max = 0;
    QPointF pos;
    for(QMap<QString, Esp>::iterator i=espMap->begin(); i!=espMap->end(); i++) {
        double x = i.value().getPosition().x();
        double y = i.value().getPosition().y();
        if (x > x_max)
            x_max = x;
        if (y > y_max)
            y_max = y;
    }
    pos.setX(x_max);
    pos.setY(y_max);

    return pos;
}

void MyServer::init(){
    confFromFile();
    maxEspCoords = setMaxEspCoords(espMap);
    DBThread dbthread(peopleMap, DBinitialized);
    if (dbthread.initialized)
        this->DBinitialized = true;
}

void MyServer::incomingConnection(qintptr socketDescriptor){
    QThread *thread = new QThread();
    ListenerThread *obj = new ListenerThread(socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap);

    obj->moveToThread(thread);

    connect(thread, SIGNAL(started()), obj, SLOT(work()));

    connect(obj, SIGNAL(ready()), this, SLOT(startToClients()));
    connect(this, SIGNAL(start2Clients()), obj, SLOT(sendStart()));
    connect(obj, &ListenerThread::log, this, &MyServer::emitLog);
    connect(obj, SIGNAL(endPackets()), this, SLOT(createElaborateThread()));

    connect(obj, SIGNAL(finished()), thread, SLOT(quit()));
    connect(obj, SIGNAL(finished()), obj, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    listenerThreadList->append(obj);

    thread->start();

}

void MyServer::createElaborateThread(){
    QMutex mutex;

    // controlla se tutte le schede hanno finito di inviare al minuto corrente
    mutex.lock();
    endPkClients ++;
    if(endPkClients<totClients){
        return;
    }

    // ricevuto end file da tutte
    endPkClients = 0;
    mutex.unlock();

    QThread *thread = new QThread();
    ElaborateThread *et = new ElaborateThread(packetsMap, packetsDetectionMap, connectedClients, peopleMap, currMinute, espMap, maxEspCoords);

    et->moveToThread(thread);

    connect(thread, SIGNAL(started()), et, SLOT(work()));

    connect(et, &ElaborateThread::log, this, &MyServer::emitLog);

    connect(et, SIGNAL(finished()), thread, SLOT(quit()));
    connect(et, SIGNAL(finished()), et, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();

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
    int i;
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
        // segnale di errore
        qDebug() << "Errore apertura file";
        emit error("Impossibile aprire il file degli esp");
        return;
    }

    inputFile >> totClients; // --- nClients è da sostituire con totClients ---
    for(i=0; i< totClients; i++){ // --- nClients è da sostituire con totClients ---
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

void MyServer::SendToDB() {
    QThread *thread = new QThread();

    // --- TODO: dbthread, per contare le persone nell'area, non deve usare peopleMap ma la lista di coordinate dall'elaborate thread
    DBThread *dbthread = new DBThread(peopleMap, DBinitialized);
    dbthread->moveToThread(thread);

    connect(thread, SIGNAL(started()), dbthread, SLOT(send()));

//    connect(dbthread, SIGNAL(ready()), this, SLOT(startToClients()));
//    connect(this, SIGNAL(start2Clients()), dbthread, SLOT(sendStart()));
//    connect(dbthread, &ListenerObj::log, this, &MyServer::emitLog
//            ,Qt::QueuedConnection
//            );

    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
//    emit DBsignal(&peopleMap);
}

void MyServer::DrawOldCountMap(QString begintime, QString endtime) {
    QThread *thread = new QThread();
    DBThread *dbthread = new DBThread(peopleMap, DBinitialized);
    QList<QPointF> *peopleCounter;
    dbthread->moveToThread(thread);

    connect(thread, SIGNAL(started()), dbthread, SLOT(&DBThread::GetTimestampsFromDB));

    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

}


void MyServer::Connects(QString slot) {
    QThread *thread = new QThread();
    DBThread *dbthread = new DBThread(peopleMap, DBinitialized);
    dbthread->moveToThread(thread);
    if (slot.compare("DrawOldCountMap")==0) {
        connect(thread, SIGNAL(started()), dbthread, SLOT(&DBThread::GetTimestampsFromDB));
    }
    else if (slot.compare("SendToDB")==0) {
        connect(thread, SIGNAL(started()), dbthread, SLOT(send()));
    }
    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

