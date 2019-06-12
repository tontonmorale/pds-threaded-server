#include "listenerthread.h"
#include "myserver.h"

ListenerThread::ListenerThread()
{

}

ListenerThread::ListenerThread(MyServer *server, qintptr socketDescriptor,
                         QMutex *mutex,
                         QMap<QString, QSharedPointer<Packet>> *packetsMap,
                         QMap<QString, int> *packetsDetectionMap,
                         QMap<QString, Esp> *espMap)
    : mutex(mutex),

      packetsMap(packetsMap),
      packetsDetectionMap(packetsDetectionMap),      
      socketDescriptor(socketDescriptor),
      espMap(espMap),
      server(server) {
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

    connect(this, &ListenerThread::addThreadSignal, server, &MyServer::addListenerThreadSlot);

    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, &ListenerThread::finished, server, &MyServer::disconnectClientSlot);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

/**
 * @brief ListenerThread::work
 * richiamata allo start del thread, setta il socket e chiama il setup iniziale dell'esp
 */
void ListenerThread::work(){
    // crea timer disconnessione e connetti segnali
    disconnectionTimer = new QTimer();
    connect(disconnectionTimer, &QTimer::timeout, this, &ListenerThread::closeConnection);

    socket = new QTcpSocket();
    if(!socket->setSocketDescriptor(socketDescriptor)){
        emit log("Errore set sock descriptor\n");
        return;
    }

    // setup iniziale dell' esp
    clientSetup();

    // imposta lo slot che reagisce alla ricezione di nuovi dati dall'esp
    connect(socket, SIGNAL(readyRead()), this, SLOT(readFromClient()));

    // segnale di ready verso il server
    emit ready();
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

    emit log("--- New connection ---");

    socket->waitForReadyRead();
    helloFromClient = QString(socket->readLine());
    mac = helloFromClient.split(" ").at(2); // mi aspetto "ciao sono <mac>\r\n"
    mac.remove('\r');
    mac.remove('\n');
    if(mac == nullptr){
        closeConnection();
    }

    // cerca id dell'esp a partire dal mac, ricavandolo dalla espMap
    id = espMap->find(mac)->getId();
    if(id == nullptr){
        closeConnection();
    }

    emit addThreadSignal(this);

    emit log("client mac: " + mac);
    hello2Client = "ciao " + id +"\r\n"; // invio "ciao <id>\r\n"
    msg = hello2Client.toStdString().c_str();
    socket->write(msg);
}

/**
 * @brief ListenerThread::sendStart
 * scrive start all'esp per iniziare l'ascolto dei pacchetti e setta il timer
 */
void ListenerThread::sendStart(){
    //start timer per rilevare disconnessioni
    disconnectionTimer->start(MyServer::intervalTime * 2 + 5000);

    socket->write("START\r\n");
    qDebug() << "mando Start";
}

/**
 * @brief ListenerThread::readFromClient
 * ricezione di nuovi dati dall'esp, geestisce il caso in cui ci sia un pacchetto nuovo o la fine dell'elenco di pacchetti
 */
void ListenerThread::readFromClient(){
    QString line, firstWord, hash, timestamp, mac, signal, microsec_str, espId, ssid;
    QStringList sl, tsSplit, macList;
//    Packet p;

//    socketTimerMap[conn]->start(MAX_WAIT+5000);

    while ( socket->canReadLine() ) {
        line = QString(socket->readLine());

        line.remove('\r');
        line.remove('\n');

        firstWord = line.split(" ").at(0);

        if(firstWord.compare("PKT")==0){
            // ricevuto nuovo pacchetto
            newPacket(line);
        }
        if(firstWord.compare("END")==0){
            // ricevuto fine dell'elenco di pacchetti
            emit endPackets();
        }

    }
}

/**
 * @brief ListenerThread::newPacket
 * ricezione di un nuovo pacchetto
 * @param line: stringa ricevuta dall'esp
 */
void ListenerThread::newPacket(QString line){
    QString hash, timestamp, mac, signal, microsec_str, espId, ssid, key, shortKey;
    Packet pkt;
    QStringList sl, tsSplit, macList;

    line.remove(',');
    sl=line.split(" ");
    if(sl.length()!=6){
        //wrong parameters number
        return;
    }

    // ricava i campi dalla stringa ricevuta
    hash = sl.at(1);
    mac = sl.at(2);
    signal = sl.at(3);
    timestamp = sl.at(4);
    espId = sl.at(5);

    emit log("[scheda " + espId + "] " + line);

    key = hash + "-" + mac + "-" + espId; // key = "pktHash-mac-espId"
    shortKey = hash + "-" + mac; // shortKey = "pktHash-mac"

    // --- inserisci nuovo pacchetto ---
    mutex->lock();

    // nuovo pacchetto
//    pkt = Packet(hash, mac, timestamp, signal, espId, "ssid"); // !!! controlla se i pacchetti rimangono nella mappa una volta uscito dalla funzione !!!
    (*packetsMap)[key] = QSharedPointer<Packet>(new Packet(hash, mac, timestamp, signal, espId, "ssid"));

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
    socket->disconnect();
    qDebug("Disconnessione client");
    emit finished(this);
}

ListenerThread::~ListenerThread(){
    delete socket;
}


