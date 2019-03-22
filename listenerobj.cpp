#include "listenerobj.h"

ListenerObj::ListenerObj()
{

}

ListenerObj::ListenerObj(qintptr socketDescriptor,
                         QMutex *m,
                         QMap<QString, QSharedPointer<Packet>> *packetsMap,
                         QMap<QString, Esp> *espMap)
    : socketDescriptor(socketDescriptor), espMap(espMap) {

//    connect(parent, SIGNAL(sig_start()),this, SLOT(sendStart()));
    this->mutex = m;
    this->packetsMap = packetsMap;    
}

void ListenerObj::work(){
    socket = new QTcpSocket();
    if(!socket->setSocketDescriptor(socketDescriptor)){
        emit log("Errore set sock descriptor\n");
        return;
    }

    clientSetup();

    connect(socket, SIGNAL(readyRead()), this, SLOT(readFromClient()));
//    qDebug() << "manda ready al server";
    emit ready();
}

void ListenerObj::sendStart(){
    socket->write("START\r\n");
//    socketTimerMap[socket]->start(MAX_WAIT+5000);
}

void ListenerObj::readFromClient(){
    QString line, firstWord, hash, timestamp, mac, signal, microsec_str, espId, ssid;
    Packet* pkt;// = new Packet();
    QStringList sl, tsSplit, macList;
//    Packet p;

//    socketTimerMap[conn]->start(MAX_WAIT+5000);

    while ( socket->canReadLine() ) {
        line = QString(socket->readLine());

        line.remove('\r');
        line.remove('\n');
        qDebug() << line;
//        emit log(line);

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
            espId = sl.at(5);

//            tsSplit = timestamp.split(':');
//            microsec_long = (tsSplit.at(0)).toLongLong()*1000000 + (tsSplit.at(1)).toLongLong();
//            microsec_str = QString::number(microsec_long);

            QString key = hash + "-" + mac + "-" + espId;
            QString shortKey = hash + "-" + mac;

            mutex->lock();

            //insert new packet
            pkt = new Packet(hash, mac, timestamp, signal, espId, "ssid");
            (*packetsMap)[key] = QSharedPointer<Packet>(pkt);

//                //update area packet count
//                if(this->areaPacketsMap->find(shortKey) != this->areaPacketsMap->end()){
//                    this->areaPacketsMap[shortKey] ++;
//                }
//                else{
//                    this->areaPacketsMap[shortKey] = 1;
//                }
            mutex->unlock();
        }
    }
}

void ListenerObj::clientSetup(){
    QStringList sl;
    QString line, clientId, hello2Client, mac, helloFromClient, id;
    const char* msg;

    qDebug() << "---New connection---\n";
//    emit log("---New connection---\n");

    // --- vecchia versione ---

    //new client, send hello and esp id
//    clientId = QString("%1").arg(connectedClients, 2, 10, QChar('0'));
//    clientId = QString("%1").arg(1, 2, 10, QChar('0'));

//    hello2Client = "CIAO " + clientId;
//    qDebug().noquote() << "Message to client: " + hello2Client;
//    hello2Client = hello2Client + "\r\n";
//    msg = hello2Client.toStdString().c_str();
//    socket->write(msg);
//    socket->waitForReadyRead();
//    helloFromClient = QString(socket->readLine());

//    helloFromClient.remove('\r');
//    helloFromClient.remove('\n');
//    qDebug().noquote() << "Received from client: " << helloFromClient;
//    mac = QString(helloFromClient).split(" ").at(3);
//    qDebug().noquote() << "Client mac: " << mac + "\n";

    // --- vecchia versione ---


    // --- nuova versione ---
    socket->waitForReadyRead();
    helloFromClient = QString(socket->readLine()); // dovrei aver ricevuto: "ciao sono <mac>"
    mac = helloFromClient.split(" ").at(2);
    mac.remove('\r');
    mac.remove('\n');
    if(mac == nullptr){
        closeConnection();
    }
    id = espMap->find(mac)->getId();
    if(id == nullptr){
        closeConnection();
    }
    qDebug() << "client mac: " + mac + "\nsending start...\n";
//    emit log("client mac: " + mac + "\nsending start...\n");
    hello2Client = "ciao " + id +"\r\n";
    msg = hello2Client.toStdString().c_str();
    socket->write(msg);

    // --- nuova versione ---


}

void ListenerObj::closeConnection(){
    socket->disconnect();
    emit finished();
}

ListenerObj::~ListenerObj(){
    delete socket;
}


