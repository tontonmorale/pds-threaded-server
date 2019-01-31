#include "myserver.h"

MyServer::MyServer(){
//    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection(socketDescriptor)));
}

void MyServer::setLog(QPlainTextEdit* log){
    this->log = log;
}

void MyServer::incomingConnection(qintptr socketDescriptor){
//    QTcpSocket *socket = this->nextPendingConnection();


    ListenerThread *thread = new ListenerThread(this, log, socketDescriptor, mutex, &packetsMap);
    connect(this, SIGNAL(sig_start()),thread, SLOT(sendStart()));
//    socket->setParent(0);
//    socket->moveToThread(thread);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

//    emit sig_start();

    //--------------------

}


