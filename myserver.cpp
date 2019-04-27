#include "mainwindow.h"
#include "myserver.h"
#include "elaboratethread.h"
#include <fstream>
#include <memory>
using namespace std;

#define ESP_FILE_PATH "C:/Users/raffy/Downloads/PDS_progetto/pds-threaded-server/esp.txt"

MyServer::MyServer(QObject *parent):
    QTcpServer (parent),
    connectedClients(0),
    totClients(1),
    currMinute(0),
    DBinitialized(false) {

    espMap = new QMap<QString, Esp>();
    mutex = new QMutex();
    listenerThreadList = new QList<ListenerThread*>();
    packetsMap = new QMap<QString, QSharedPointer<Packet>>();
    packetsDetectionMap = new QMap<QString, int>;
    peopleMap = new QMap<QString, Person>;
    devicesCoords = new QList<QPointF>;
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
    ListenerThread *lt = new ListenerThread(socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap);

    lt->moveToThread(thread);
    lt->signalsConnection(thread, this);
    listenerThreadList->append(lt);
    thread->start();
}

void MyServer::createElaborateThread(){
    QMutex mutex;

    // controlla se ricevuto endPackets da tutte le schede
    mutex.lock();
    endPkClients ++;
    if(endPkClients<totClients){
        return;
    }

    // ricevuto end file da tutte le schede
    endPkClients = 0;
    mutex.unlock();

    QThread *thread = new QThread();
    ElaborateThread *et = new ElaborateThread(packetsMap, packetsDetectionMap, connectedClients,
                                              peopleMap, currMinute, espMap, maxEspCoords, devicesCoords);
    et->moveToThread(thread);
    et->signalsConnection(thread, this);
    thread->start();

    // inizio prossimo minuto
    startToClients();
}

void MyServer::emitLog(QString message){
    emit log(message);
}

void MyServer::readyFromClient(){
    connectedClients++;
    if(connectedClients==totClients
//            && connectedClients>=3
            ){
        currMinute = 0;
        startToClients();
    }

}

void MyServer::startToClients(){
    // manda start se:
    //  -inizialmente tutti i client sono connessi
    //  -le volte successive se almeno 3 client connessi
    if(connectedClients==totClients
//            && connectedClients>=3
            )
    {
        qDebug() << "Start to clients\n";
        endPkClients = 0;
        emit log("\n[server] sending start...\n");
        currMinute++;
        emit start2Clients();
    }
    else {
        // todo: meno di 3 client connessi => ?????
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

void MyServer::dataForDb(){
    // todo: passare dati al thread che gestisce il db
//    SendToDB(*devicesCoords);

    // pulisce le strutture dati per il time slot successivo
    packetsMap->clear();
    packetsDetectionMap->clear();
    peopleMap->clear();
    devicesCoords->clear();
}

void MyServer::SendToDB() {
    QThread *thread = new QThread();

    // --- TODO: dbthread, per contare le persone nell'area, non deve usare peopleMap ma la lista di coordinate dall'elaborate thread
    DBThread *dbthread = new DBThread(peopleMap, DBinitialized);
    dbthread->moveToThread(thread);

    dbthread->signalsConnection(thread, this);

//    connect(dbthread, SIGNAL(ready()), this, SLOT(startToClients()));
//    connect(this, SIGNAL(start2Clients()), dbthread, SLOT(sendStart()));
//    connect(dbthread, &ListenerObj::log, this, &MyServer::emitLog
//            ,Qt::QueuedConnection
//            );

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

MyServer::~MyServer() {
//    espMap = new QMap<QString, Esp>();
//    mutex = new QMutex();
//    listenerThreadList = new QList<ListenerThread*>();
//    packetsMap = new QMap<QString, QSharedPointer<Packet>>();
//    packetsDetectionMap = new QMap<QString, int>;
//    peopleMap = new QMap<QString, Person>;
//    devicesCoords = new QList<QPointF>;
}
