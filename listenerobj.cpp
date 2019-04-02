#include "listenerobj.h"

ListenerObj::ListenerObj()
{

}

ListenerObj::ListenerObj(qintptr socketDescriptor,
                         QMutex *mutex,
                         QMap<QString, QSharedPointer<Packet>> *packetsMap,
                         QMap<QString, int> *packetsDetectionMap,
                         QMap<QString, Esp> *espMap)
    : mutex(mutex),
      packetsMap(packetsMap),
      packetsDetectionMap(packetsDetectionMap),
      socketDescriptor(socketDescriptor),
      espMap(espMap){
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

void ListenerObj::clientSetup(){
    QStringList sl;
    QString line, clientId, hello2Client, mac, helloFromClient, id;
    const char* msg;

    emit log("--- New connection ---");

    socket->waitForReadyRead();
    helloFromClient = QString(socket->readLine());
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
    emit log("client mac: " + mac);
    hello2Client = "ciao " + id +"\r\n";
    msg = hello2Client.toStdString().c_str();
    socket->write(msg);
}

void ListenerObj::sendStart(){
    socket->write("START\r\n");
//    socketTimerMap[socket]->start(MAX_WAIT+5000);
}

void ListenerObj::readFromClient(){
    QString line, firstWord, hash, timestamp, mac, signal, microsec_str, espId, ssid;
    QStringList sl, tsSplit, macList;
//    Packet p;

//    socketTimerMap[conn]->start(MAX_WAIT+5000);

    while ( socket->canReadLine() ) {
        line = QString(socket->readLine());

        line.remove('\r');
        line.remove('\n');

        firstWord = line.split(" ").at(0);

        //received new packet
        if(firstWord.compare("PKT")==0){
            newPacket(line);
        }
        if(firstWord.compare("END")==0){
            emit endPackets();
        }

    }
}

void ListenerObj::newPacket(QString line){
    QString hash, timestamp, mac, signal, microsec_str, espId, ssid, key, shortKey;
    Packet pkt;
    QStringList sl, tsSplit, macList;

    line.remove(',');
    sl=line.split(" ");
    if(sl.length()!=6){
        //wrong parameters number
        return;
    }

    hash = sl.at(1);
    mac = sl.at(2);
    signal = sl.at(3);
    timestamp = sl.at(4);
    espId = sl.at(5);

    emit log("[scheda " + espId + "] " + line);

    key = hash + "-" + mac + "-" + espId;
    shortKey = hash + "-" + mac;

    // --- update packets ---
    mutex->lock();

    //insert new packet
    pkt = Packet(hash, mac, timestamp, signal, espId, "ssid"); // !!! controlla se i packetti rimangono nella mappa una volta uscito dalla funzione !!!
    (*packetsMap)[key] = QSharedPointer<Packet>(&pkt);

    //update detection packets count
    if(packetsDetectionMap->find(shortKey) != packetsDetectionMap->end()){
        (*packetsDetectionMap)[shortKey] ++;
    }
    else{
        (*packetsDetectionMap)[shortKey] = 1;
    }
    mutex->unlock();
    // --- update packets ---
}

void ListenerObj::closeConnection(){
    socket->disconnect();
    emit finished();
}

ListenerObj::~ListenerObj(){
    delete socket;
}


