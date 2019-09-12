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
                               int& totClients)
    : endPacketSent(false),
      mutex(mutex),
      packetsMap(packetsMap),
      packetsDetectionMap(packetsDetectionMap),      
      socketDescriptor(socketDescriptor),
      espMap(espMap),      
      server(server),
      maxSignal(maxSignal),
      totClients(totClients),
      firstStart(true),
      tag("ListenerThread") {
}

/**
 * @brief ListenerThread::signalsConnection
 * connette segnali e slot di questo thread col server
 * @param thread
 */
void ListenerThread::signalsConnection(QThread *thread){

    connect(thread, SIGNAL(started()), this, SLOT(work()));

    connect(this, &ListenerThread::ready, server, &MyServer::readyFromClientSlot);
    connect(server, &MyServer::start2ClientsSig, this, &ListenerThread::sendStart);
    connect(this, &ListenerThread::log, server, &MyServer::emitLogSlot);
    connect(this, &ListenerThread::endPackets, server, &MyServer::createElaborateThreadSlot);
//    connect(server, &MyServer::closeConnectionSig, this, &ListenerThread::closeConnection);

//    connect(this, &ListenerThread::finished, thread, &QThread::quit);
    connect(this, &ListenerThread::finished, server, &MyServer::disconnectClientSlot);
    connect(this, &ListenerThread::beforeDestructionSig, this, &ListenerThread::beforeDestructionSlot);
//    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
//    connect(this, &ListenerThread::finished, this, &ListenerThread::deleteLater);
}

void ListenerThread::beforeDestructionSlot(){
    disconnectionTimer->stop();
}

/**
 * @brief ListenerThread::work
 * richiamata allo start del thread, setta il socket e chiama il setup iniziale dell'esp
 */
void ListenerThread::work(){
    // crea timer disconnessione e connetti segnali
    try {
        disconnectionTimer = new QTimer(this);
        connect(disconnectionTimer, &QTimer::timeout, this, &ListenerThread::closeConnection,  Qt::DirectConnection);
    } catch (bad_alloc e) {
        emit log(tag + ": Errore nell'allocazione del disconnection timer, non proseguo.");
        return;
    }

    try {
        socket = new QTcpSocket();
        if(!socket->setSocketDescriptor(socketDescriptor)){
            throw("Errore nel set socket descriptor\n");
        }
    } catch (bad_alloc e) {
       emit log(tag + ": Errore nell'allocazione del socket, non proseguo.");
        return;
    } catch (exception e) {
        emit log(tag + ": " + e.what());
        return;
    }


    // setup iniziale dell' esp
    clientSetup();

    // imposta lo slot che reagisce alla ricezione di nuovi dati dall'esp
    connect(socket, SIGNAL(readyRead()), this, SLOT(readFromClient()));

    // segnale di ready verso il server
    emit ready(this);
}

QString ListenerThread::getId(){
    return id;
}

/**
 * @brief ListenerThread::clientSetup
 * riceve il mac dall'esp e gli invia l'id letto da file
 */
void ListenerThread::clientSetup(){
    QStringList sl;
    QString line, clientId, hello2Client, mac, helloFromClient;
    const char* msg;

    try {
        socket->waitForReadyRead();
        helloFromClient = QString(socket->readLine());
        mac = helloFromClient.split(" ").at(2); // mi aspetto "ciao sono <mac>\r\n"
        mac.remove('\r');
        mac.remove('\n');
        if(mac == nullptr){
            closeConnection();
        }

        // cerca id dell'esp a partire dal mac, ricavandolo dalla espMap
        auto it = espMap->find(mac);
        if(it != espMap->end())
            id = it.value().getId();
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
    } catch (out_of_range e) {
        emit log(tag + ": problemi di out of range nella clientSetUp");
        //anche qui, che fare?
    } catch (exception e) {
        emit log(tag + ": Probabile errore nel socket");
        //TODO: che faccio se ho un errore sul socket?
    }

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
            qDebug() << tag << ": mando Start a " << id << "(minuto " << currMinute << ")";

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
                emit log(tag + ": Impossibile mandare start, probabili errori sul socket, considero il client disconnesso."); //possibile soluzione
                closeConnection();
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
//    Packet p;

//    socketTimerMap[conn]->start(MAX_WAIT+5000);
    QString toLog = "disconnectionTimer scheda " + id;
    toLog += " time left: ";
    toLog += QString::number(disconnectionTimer->remainingTime());


    //start timer per rilevare disconnessioni
    disconnectionTimer->start(MyServer::intervalTime + 5000);

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
        emit log("Impossibile leggere dal socket, probabili errori sul socket, considero il client disconnesso.");
        closeConnection();
        return;
    }
}

/**
 * @brief ListenerThread::newPacket
 * ricezione di un nuovo pacchetto
 * @param line: stringa ricevuta dall'esp
 */
void ListenerThread::newPacket(QString line){
    QString hash, timestamp, mac, microsec_str, espId, ssid, key, shortKey;
    Packet pkt;
    QStringList sl, tsSplit, macList;
    int signal;

    line.remove(',');
    sl=line.split(" ");
    if(sl.length()!=6){
        //wrong parameters number
        return;
    }

    // ricava i campi dalla stringa ricevuta
    hash = sl.at(1);
    mac = sl.at(2);
    signal = sl.at(3).toInt();
    timestamp = sl.at(4);
    espId = sl.at(5);

    if(mac.compare("30:74:96:94:e3:2d")==0 || mac.compare("94:65:2d:41:f7:8c")==0 )
        emit log(tag + ": " + "[esp " + espId + "] " + line);

    //scarto pacchetto se valore intensità segnale sballata
//    if(abs(signal) > abs(maxSignal)){
//        return;
//    }

    key = hash + "-" + mac + "-" + espId; // key = "pktHash-mac-espId"
    shortKey = hash + "-" + mac; // shortKey = "pktHash-mac"

    // --- inserisci nuovo pacchetto ---
    mutex->lock();

    // nuovo pacchetto
//    pkt = Packet(hash, mac, timestamp, signal, espId, "ssid"); // !!! controlla se i pacchetti rimangono nella mappa una volta uscito dalla funzione !!!
    (*packetsMap)[key] = Packet(hash, mac, timestamp, signal, espId, "ssid");

    // aggiorna packetsDetectionMap, aggiorna il conto di quante schede hanno rilevato ogni pacchetto
    if(packetsDetectionMap->find(shortKey) != packetsDetectionMap->end()){
        (*packetsDetectionMap)[shortKey] ++;
    }
    else{
        (*packetsDetectionMap)[shortKey] = 1;
    }
    mutex->unlock();
    // --- inserisci nuovo pacchetto ---
}

void ListenerThread::closeConnection(){
//    socket->write("DISCONNECTED\r\n");

    qDebug() << tag << ": Disconnessione client";
    emit finished(id);
}

ListenerThread::~ListenerThread(){
    qDebug() << tag << ": Distruttore ListenerThread";
    socket->disconnect();
    disconnectionTimer->deleteLater();
    socket->deleteLater();
    socket = nullptr;
}


