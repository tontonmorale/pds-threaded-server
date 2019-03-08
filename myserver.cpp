#include "myserver.h"

MyServer::MyServer(QObject *parent): QTcpServer (parent){
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
}

void MyServer::setLog(QPlainTextEdit* log){
    this->log = log;
}

void MyServer::incomingConnection(qintptr socketDescriptor){
//    QTcpSocket *socket = this->nextPendingConnection();
//    socket -> deleteLater();

    QThread *thread = new QThread();
    ListenerObj *obj = new ListenerObj(log, socketDescriptor, mutex, &packetsMap);

    obj->moveToThread(thread);

    connect(thread, SIGNAL (started()), obj, SLOT (process()));

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
    // controlla se tutti i client sono connessi
    // --- todo ---

    emit sig_start();
}


