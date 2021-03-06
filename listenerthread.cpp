#include "listenerthread.h"
#include "myserver.h"
#include "math.h"

ListenerThread::ListenerThread(int& totClients): totClients(totClients)
{

}

ListenerThread::ListenerThread(MyServer *server,
                               qintptr socketDescriptor,
                               QMutex *mutex,
                               QMap<QString, Packet> *packetsMap,
                               QMap<QString, int> *packetsDetectionMap,
                               QMap<QString, Esp> *espMap,
                               double maxSignal,
                               int& totClients,
                               int* currMinute)
    : endPacketSent(false),
      id(""),
      mac(""),
      mutex(mutex),
      packetsMap(packetsMap),
      packetsDetectionMap(packetsDetectionMap),      
      socketDescriptor(socketDescriptor),      
      espMap(espMap),      
      server(server),      
      maxSignal(maxSignal),
      totClients(totClients),
      firstStart(true),      
      tag("ListenerThread"),
      currMinute(currMinute){

}

/**
 * @brief ListenerThread::signalsConnection
 * connette segnali e slot di questo thread col server
 * @param thread
 */
void ListenerThread::signalsConnection(QThread *thread){
    this->thread = thread;

    connect(thread, SIGNAL(started()), this, SLOT(work()));

    connect(this, &ListenerThread::ready, server, &MyServer::readyFromClientSlot);

    connect(this, &ListenerThread::log, server, &MyServer::emitLogSlot);
    connect(this, &ListenerThread::endPackets, server, &MyServer::createElaborateThreadSlot);

    connect(server, &MyServer::closeConnectionSig, this, &ListenerThread::closeConnection);

    connect(this, &ListenerThread::finished, server, &MyServer::disconnectClientSlot);

    connect(thread, &QThread::finished, thread, &QThread::quit);
}

/**
 * @brief ListenerThread::work
 * richiamata allo start del thread, setta il socket e chiama il setup iniziale dell'esp
 */
void ListenerThread::work(){
    // crea timer disconnessione e connetti segnali
    try {
        disconnectionTimer = new QTimer(this);
        connect(disconnectionTimer, &QTimer::timeout, this, &ListenerThread::beforeClosingSlot,  Qt::DirectConnection);
    } catch (bad_alloc e) {
        emit log(tag + ": errore nell'allocazione del disconnection timer, disconenssione", "red");
        return;
    }

    try {
        socket = new QTcpSocket();
        if(!socket->setSocketDescriptor(socketDescriptor)){
            throw("Errore nel set socket descriptor\n");
        }
    } catch (bad_alloc e) {
       emit log(tag + ": Errore nell'allocazione del socket, disconnessione client " + id, "red");
        return;
    } catch (exception e) {
        emit log(tag + ": disconnessione client " + id, "red");
        return;
    }


    // setup iniziale dell' esp
    clientSetup();

    // imposta lo slot che reagisce alla ricezione di nuovi dati dall'esp
    connect(socket, SIGNAL(readyRead()), this, SLOT(readFromClient()));
    connect(server, &MyServer::start2ClientsSig, this, &ListenerThread::sendStart);

    // segnale di ready verso il server
    emit ready(this);
}

QString ListenerThread::getId(){
    return id;
}

QString ListenerThread::getMac(){
    return mac;
}

void ListenerThread::beforeClosingSlot(){
    if(id.compare("")==0)
        this->deleteLater();
    else
        emit finished(id);
}

/**
 * @brief ListenerThread::clientSetup
 * riceve il mac dall'esp e gli invia l'id letto da file
 */
void ListenerThread::clientSetup(){
    QStringList sl;
    QString line, clientId, hello2Client, helloFromClient;
    const char* msg;
    mutex->lock();
    try {
        socket->waitForReadyRead();
        helloFromClient = QString(socket->readLine());
        mac = helloFromClient.split(" ").at(2); // mi aspetto "ciao sono <mac>\r\n"
        mac.remove('\r');
        mac.remove('\n');
        if(mac == nullptr){
            emit finished(id);
            mutex->unlock();
            return;
        }

        // cerca id dell'esp a partire dal mac, ricavandolo dalla espMap
        qDebug().noquote() << tag << "find su espMap";
        auto it = espMap->find(mac);
        if(it != espMap->end()){
            qDebug().noquote() << tag << "trovato esp un espMap";
            id = it.value().getId();
        }
        else{

            totClients ++;

            if(totClients<10){
                id = "0";
                id += QString::number(totClients);
            }
            else{
                id = QString::number(totClients);
            }

            // inserisco nuovo client nella mappa
            espMap->insert(mac, Esp(id, mac, QPointF(NAN, NAN)));
        }
        hello2Client = "ciao " + id +"\r\n"; // invio "ciao <id>\r\n"
        msg = hello2Client.toStdString().c_str();
        socket->write(msg);
        disconnectionTimer->start(3*MyServer::intervalTime);
    } catch (exception e) {
        emit log(tag + ": Probabile errore nel socket", "red");
        mutex->unlock();
        if(id.compare("")!=0)
            emit finished(id);
        else
            closeConnection(id);

    }
    mutex->unlock();
}

/**
 * @brief ListenerThread::sendStart
 * scrive start all'esp per iniziare l'ascolto dei pacchetti e setta il timer di disconnessione
 */
void ListenerThread::sendStart(int currMinute){

//    endPacketSent = false;
    int retry = 3;
    while (retry) {
        try {
            mutex->lock();
            socket->write("START\r\n");
            qDebug() << tag << ": mando Start a " << id;

            if(firstStart){
                //start timer per rilevare disconnessioni
                disconnectionTimer->start(2*MyServer::intervalTime + 2000);
                firstStart = false;
            }
            mutex->unlock();
            break;
        } catch (...) {
            retry--;
            if (!retry) {
                emit log(tag + ": Impossibile mandare start, probabili errori sul socket, disconnessione client " + id, "red"); //possibile soluzione
                emit finished(id);
            }
            mutex->unlock();
        }
    }
}

bool ListenerThread::getEndPacketSent(){
    return endPacketSent;
}

/**
 * @brief ListenerThread::readFromClient
 * ricezione di nuovi dati dall'esp, geestisce il caso in cui ci sia un pacchetto nuovo o la fine dell'elenco di pacchetti
 * se errore => chiude connessione
 */
void ListenerThread::readFromClient(){
    QString line, firstWord, hash, timestamp, mac, signal, microsec_str, espId, ssid;
    QStringList sl, tsSplit, macList;

    QString toLog = "disconnectionTimer scheda " + id;
    toLog += " time left: ";
    toLog += QString::number(disconnectionTimer->remainingTime());


    //start timer per rilevare disconnessioni
    disconnectionTimer->start(2*MyServer::intervalTime + 2000);

    try{
        while ( socket->canReadLine() ) {
            line = QString(socket->readLine());

            qDebug() << tag << ": " << line;

            line.remove('\r');
            line.remove('\n');

            firstWord = line.split(" ").at(0);

            if(firstWord.compare("PKT")==0){
                // ricevuto nuovo pacchetto
                newPacket(line);
            }
            if(firstWord.compare("END")==0){
                // ricevuto fine dell'elenco di pacchetti
                qDebug() << tag << ": end ricevuto da: " << id;
                endPacketSent = true;
                emit endPackets();
            }
        }
    } catch (...) {
        // errore socket, disconnetto
        emit log("Impossibile leggere dal socket, probabili errori sul socket, disconnessione client " + id, "red");
        emit finished(id);
        return;
    }
}

/**
 * @brief ListenerThread::newPacket
 * estrae dati dal nuovo pacchetto ricevuto
 * @param line: stringa ricevuta dall'esp
 */
void ListenerThread::newPacket(QString line){
    QString hash, timestamp, mac, microsec_str, espId, ssid, key, shortKey;
    Packet pkt;
    QStringList sl, tsSplit, macList;
    int signal;
    try{

        line.remove(',');
        sl=line.split(" ");
        if(sl.length()!=6){
            //wrong parameters number
            mutex->unlock();
            return;
        }

        // ricava i campi dalla stringa ricevuta
        hash = sl.at(1);
        mac = sl.at(2);
        signal = sl.at(3).toInt();
        timestamp = sl.at(4);
        espId = sl.at(5);

        emit log(tag + ": " + "[esp " + espId + "] " + line, "black");

        key = hash + "-" + mac + "-" + espId; // key = "pktHash-mac-espId"
        shortKey = hash + "-" + mac; // shortKey = "pktHash-mac"

        // inserisci nuovo pacchetto
        mutex->lock();

        // nuovo pacchetto
        (*packetsMap)[key] = Packet(hash, mac, timestamp, signal, espId, "ssid");

        // aggiorna packetsDetectionMap, aggiorna il conto di quante schede hanno rilevato ogni pacchetto
        if(packetsDetectionMap->find(shortKey) != packetsDetectionMap->end()){
            (*packetsDetectionMap)[shortKey] ++;
        }
        else{
            (*packetsDetectionMap)[shortKey] = 1;
        }
    }catch(...) {
        qDebug() << tag << ": problema in closeConnection";
        mutex->unlock();
    }

    mutex->unlock();
}

// chiude connessione: ferma timer e distrugge socket
void ListenerThread::closeConnection(QString id){
    try{
        if(this->id.compare(id)==0){
            qDebug() << tag << ": Disconnessione client";
            disconnectionTimer->stop();
            socket->disconnect();
            socket->deleteLater();
            this->deleteLater();
            thread->quit();
            thread->deleteLater();
        }
    } catch(...) {
        qDebug() << tag << ": problema in closeConnection";
    }
}

ListenerThread::~ListenerThread(){
    qDebug() << tag << ": Distruttore ListenerThread";

}


