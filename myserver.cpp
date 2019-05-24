#include "mainwindow.h"
#include "myserver.h"
#include <fstream>
#include <memory>
using namespace std;

//#define ESP_FILE_PATH "C:/Users/raffy/Desktop/PDS_prova/pds-threaded-server/esp.txt"
#define ESP_FILE_PATH "C:/Users/tonio/Desktop/pds-threaded-server/esp.txt"

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

/**
 * @brief MyServer::setMaxEspCoords
 * cerca x e y max tra le coordinate degli esp
 * @param espMap, mappa degli esp
 * @return QPoint con le coord max
 */
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

/**
 * @brief MyServer::init
 * legge le posizioni degli esp da file, cerca le loro coord max e inizializza il db
 */
void MyServer::init(){
    QThread *thread = new QThread();
    DBThread *dbthread = new DBThread(this);

    dbthread->moveToThread(thread);
    dbthread->signalsConnection(thread);
    thread->start();

    confFromFile();
    maxEspCoords = setMaxEspCoords(espMap);
}

/**
 * @brief MyServer::incomingConnection
 * genera un listenerThread all'arrivo di una nuova connessione
 * @param socketDescriptor: descrittore del socket in ascolto
 */
void MyServer::incomingConnection(qintptr socketDescriptor){
    QThread *thread = new QThread();
    ListenerThread *lt = new ListenerThread(this, socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap);

    lt->moveToThread(thread);
    lt->signalsConnection(thread);
    listenerThreadList->append(lt);
    thread->start();
}

/**
 * @brief MyServer::createElaborateThread
 * -> controlla se ha ricevuto il segnale di "pacchetti ricevuti" da tutti i listenerThread
 * -> genera l'elaborateThread per ottenere i device nell'area nel minuto passato
 * -> manda segnale di start per il nuovo minuto
 */
void MyServer::createElaborateThreadSlot(){
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
    ElaborateThread *et = new ElaborateThread(this, packetsMap, packetsDetectionMap, connectedClients,
                                              peopleMap, currMinute, espMap, maxEspCoords, devicesCoords);
    et->moveToThread(thread);
    et->signalsConnection(thread);
    thread->start();

    // inizio prossimo minuto
    startToClients();
}

void MyServer::emitLogSlot(QString message){
    emit logSig(message);
}

/**
 * @brief MyServer::readyFromClient
 * -> slot che risponde al segnale di completamento del setup di un esp
 * -> se ricevuto da tutti gli esp, manda segnale di start del blocco di N minuti
 */
void MyServer::readyFromClientSlot(){
    connectedClients++;
    if(connectedClients==totClients
//            && connectedClients>=3
            ){
        currMinute = 0;
        // inizio blocco N minuti
        startToClientsSlot();
    }

}

/**
 * @brief MyServer::startToClients
 * manda start agli esp se:
    -> inizialmente tutti i client sono connessi
    -> le volte successive se almeno 3 client connessi
 */
void MyServer::startToClientsSlot(){
    if(connectedClients==totClients
//            && connectedClients>=3
            )
    {
        qDebug() << "Start to clients\n";
        endPkClients = 0;
        emit logSig("\n[server] Current minute " + QString::number(currMinute) + ": sending start...\n");
        currMinute++;
        emit start2ClientsSig();
    }
    else {
        // todo: meno di 3 client connessi => ?????
    }
}

void MyServer::errorFromThreadSlot(QString errorMsg){
    emit fatalErrorSig(errorMsg);
}

/**
 * @brief MyServer::confFromFile
 * legge posizioni degli esp da file
 */
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
        emit fatalErrorSig("Impossibile aprire il file degli esp");
        return;
    }

    inputFile >> totClients;
    for(i=0; i< totClients; i++){
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

/**
 * @brief MyServer::dataForDb
 */
void MyServer::dataForDb(){
    // todo: passare dati al thread che gestisce il db
    SendToDB();

    // pulisce le strutture dati per il time slot successivo
//    packetsMap->clear();
    packetsMap = new QMap<QString, QSharedPointer<Packet>>(); // todo: da rivedere
    packetsDetectionMap->clear();

    devicesCoords->clear();
}

void MyServer::clearPeopleMapSlot(){
    peopleMap->clear();
}



/**
 * @brief MyServer::SendToDB
 */
void MyServer::SendToDB() {
//    QThread *thread = new QThread();
//    DBThread *dbthread = new DBThread(this, peopleMap, devicesCoords->size(), DBinitialized, "", "", nullptr);

//    dbthread->moveToThread(thread);

//    dbthread->signalsConnection(thread, "SendToDB");

    thread->start(); // diventa chiamata alla funzione giusta
//    connect(dbthread, SIGNAL(ready()), this, SLOT(startToClients()));
//    connect(this, SIGNAL(start2Clients()), dbthread, SLOT(sendStart()));
//    connect(dbthread, &ListenerObj::log, this, &MyServer::emitLog
//            ,Qt::QueuedConnection
//            );


//    emit DBsignal(&peopleMap);
}

/**
 * @brief MyServer::DrawOldCountMap
 * @param begintime
 * @param endtime
 * @return
 */
QList<QPointF> *MyServer::DrawOldCountMap(QString begintime, QString endtime) {
    QThread *thread = new QThread();
    QList<QPointF> *peopleCounter = new QList<QPointF>();
    DBThread *dbthread = new DBThread(this, peopleMap, devicesCoords->size(), DBinitialized, begintime, endtime, peopleCounter);

    dbthread->moveToThread(thread);
    dbthread->signalsConnection(thread, "DrawOldCountMap");

    thread->start();

    //controlla
    return peopleCounter;

//    connect(thread, SIGNAL(started()), dbthread, SLOT(GetTimestampsFromDB()));

//    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
//    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
//    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));


}

//void MyServer::Connects(QString slot) {
//    QThread *thread = new QThread();
//    DBThread *dbthread = new DBThread(peopleMap, devicesCoords->size(), DBinitialized);
//    dbthread->moveToThread(thread);
//    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
//    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
//    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
//}

MyServer::~MyServer() {
//    espMap = new QMap<QString, Esp>();
//    mutex = new QMutex();
//    listenerThreadList = new QList<ListenerThread*>();
//    packetsMap = new QMap<QString, QSharedPointer<Packet>>();
//    packetsDetectionMap = new QMap<QString, int>;
//    peopleMap = new QMap<QString, Person>;
//    devicesCoords = new QList<QPointF>;
}
