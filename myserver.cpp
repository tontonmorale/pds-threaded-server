#include "myserver.h"

MyServer::MyServer(){
    connect(this, SIGNAL(newConnection()), this, SLOT(onClientConnection()));
}

void MyServer::setLog(QPlainTextEdit* log){
    this->log = log;
}

void MyServer::onClientConnection(){
    QTcpSocket *socket = this->nextPendingConnection();

    //----------------------
    clientSetup(socket);

    //---------------------

    ListenerThread *thread = new ListenerThread(this, log, socket);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
    emit sig_start();

    //--------------------

}


