#include "myserver.h"
#include <fstream>
#include <memory>
using namespace std;

#define ESP_FILE_PATH "C:/Users/tonio/Desktop/ServerPds/esp.txt"

MyServer::MyServer(QObject *parent): QTcpServer (parent), connectedClients(0){
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
    espMap = new QMap<QString, Esp>();
    mutex = new QMutex();
    objList = new QList<ListenerObj*>();
    totClients = 1;
}

void MyServer::init(){
    confFromFile();
}

void MyServer::setLog(QPlainTextEdit* log){
    this->log = log;
}

void MyServer::incomingConnection(qintptr socketDescriptor){
//    QTcpSocket *socket = this->nextPendingConnection();
//    socket -> deleteLater();

    QThread *thread = new QThread();
    ListenerObj *obj = new ListenerObj(socketDescriptor, mutex, &packetsMap, espMap);

    obj->moveToThread(thread);

    connect(thread, SIGNAL(started()), obj, SLOT(work()));

    connect(obj, SIGNAL(ready()), this, SLOT(startToClients()));
    connect(this, SIGNAL(start2Clients()), obj, SLOT(sendStart()));
    connect(obj, &ListenerObj::log, parent, &MyServer::printToLog
//            ,Qt::QueuedConnection
            );

    connect(obj, SIGNAL(finished()), thread, SLOT(quit()));
    connect(obj, SIGNAL(finished()), obj, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    objList->append(obj);

    thread->start();



    //--------------------

}

void MyServer::printToLog(QString message){
    log->insertPlainText(message);
}

void MyServer::startToClients(){
    // manda start se tutti i client sono connessi
    connectedClients++;
    if(connectedClients==totClients
//            && connectedClients>=3
            ){
        log->insertPlainText("Start to clients\n");
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
        // --- emettere segnale di errore
        emit error("Impossibile aprire il file degli esp");
        log->insertPlainText("Errore apertura file");
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



