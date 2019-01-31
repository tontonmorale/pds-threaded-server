#include "listenerthread.h"

ListenerThread::ListenerThread()
{

}

ListenerThread::ListenerThread(QObject *parent, QPlainTextEdit *log, qintptr socketDescriptor, QMutex *m, QMap<QString, QSharedPointer<Packet>> *packetsMap)
    : QThread(parent)/*, socket(socket) */{

//    connect(parent, SIGNAL(sig_start()),this, SLOT(sendStart()));


//    this->socket = new QTcpSocket();
    this->socketDescriptor = socketDescriptor;
    this->log = log;
    this->mutex = m;
    this->packetsMap = packetsMap;
}

void ListenerThread::run(){
    socket = new QTcpSocket();
    socket->setSocketDescriptor(socketDescriptor);
    clientSetup();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readFromClient()));
    exec();
    //aspetta start dal server --> da rivedere
//    while(1){

//    }
}

void ListenerThread::sendStart(){

    qDebug().noquote() << "invio start";
    socket->write("START\r\n");
//    socketTimerMap[socket]->start(MAX_WAIT+5000);
}

void ListenerThread::readFromClient(){
    QString line, firstWord, hash, timestamp, mac, signal, microsec_str, esp, ssid;
    Packet* pkt;// = new Packet();
    QStringList sl, tsSplit, macList;
//    Packet p;

//    socketTimerMap[conn]->start(MAX_WAIT+5000);

//    while ( socket->canReadLine() ) {
        line = QString(socket->readLine());
        line.remove('\n');
        qDebug().noquote() << line + "\n";

        firstWord = line.split(" ").at(0);

        //received new packet
        if(firstWord.compare("PKT")==0){

            line.remove(',');
            sl=line.split(" ");
            if(sl.length()!=6){
                //wrong parameters number
//                continue;
            }

            hash = sl.at(1);
            mac = sl.at(2);
            signal = sl.at(3);
            timestamp = sl.at(4);
            esp = sl.at(5);

//            tsSplit = timestamp.split(':');
//            microsec_long = (tsSplit.at(0)).toLongLong()*1000000 + (tsSplit.at(1)).toLongLong();
//            microsec_str = QString::number(microsec_long);

            QString key = hash + "-" + mac + "-" + esp;
            QString shortKey = hash + "-" + mac;

            mutex->lock();
//            if(this->packetsMap->find(key) == this->packetsMap->end()){
            //controllare se funziona
            if (packetsMap->contains(key)) {
                //insert new packet
                pkt = new Packet(hash, mac, timestamp, signal, esp, "ssid");
                (*packetsMap)[key] = QSharedPointer<Packet>(pkt);

//                //update area packet count
//                if(this->areaPacketsMap->find(shortKey) != this->areaPacketsMap->end()){
//                    this->areaPacketsMap[shortKey] ++;
//                }
//                else{
//                    this->areaPacketsMap[shortKey] = 1;
//                }
//            }
            mutex->unlock();
        }
    }
}

void ListenerThread::clientSetup(){
    QStringList sl;
    QString line, clientId, hello2Client, mac, helloFromClient;
    const char* msg;

    qDebug().noquote() << "NEW CONNECTION\nClient number: 1"; /*QString::number(this->connectedClients);*/

    //new client, send hello and esp id
//    clientId = QString("%1").arg(connectedClients, 2, 10, QChar('0'));
    clientId = QString("%1").arg(1, 2, 10, QChar('0'));

    hello2Client = "CIAO " + clientId;
    qDebug().noquote() << "Message to client: " + hello2Client;
    hello2Client = hello2Client + "\r\n";
    msg = hello2Client.toStdString().c_str();
    socket->write(msg);
    socket->waitForReadyRead();
    helloFromClient = QString(socket->readLine());

    helloFromClient.remove('\r');
    helloFromClient.remove('\n');
    qDebug().noquote() << "Received from client: " << helloFromClient;
    mac = QString(helloFromClient).split(" ").at(3);
    qDebug().noquote() << "Client mac: " << mac + "\n";

}


