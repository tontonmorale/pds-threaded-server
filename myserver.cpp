#include "myserver.h"
#include <fstream>
#include <memory>
using namespace std;

//#define ESP_FILE_PATH "C:/Users/tonio/Desktop/ServerPds/esp.txt"
#define ESP_FILE_PATH "C:/Users/tonio/ServerPds/esp.txt"

MyServer::MyServer(QObject *parent): QTcpServer (parent){
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
    QMap<QString, Esp> qmap;
    shared_ptr<QMap<QString, Esp>> espMap(&qmap);
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
    ListenerObj *obj = new ListenerObj(log, socketDescriptor,
                                       mutex, &packetsMap,
                                       espMap);

    obj->moveToThread(thread);

    connect(thread, SIGNAL (started()), obj, SLOT (start()));

    connect(obj, SIGNAL(ready()), this, SLOT(startToClients()));
    connect(this, SIGNAL(sig_start()), obj, SLOT(sendStart()));

    connect(obj, SIGNAL (finished()), thread, SLOT (quit()));
    connect(obj, SIGNAL (finished()), obj, SLOT (deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    qDebug().noquote() << QThread::currentThread();

    thread->start();



    //--------------------

}

void MyServer::startToClients(){
    // manda start se tutti i client sono connessi
    // --- todo ---

    emit sig_start();
}

void MyServer::confFromFile(){
    ifstream inputFile;
    int i, n_esp;
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
        qDebug() << "Errore apertura file";
        return;
    }

    inputFile >> n_esp;
    for(i=0; i< n_esp; i++){
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



