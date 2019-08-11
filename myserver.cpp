#include "mainwindow.h"
#include "myserver.h"
#include "utility.h"
#include <fstream>
#include <memory>
using namespace std;

//#define ESP_FILE_PATH "C:/Users/raffy/Desktop/PDS_prova/pds-threaded-server/esp.txt"
#define ESP_FILE_PATH "C:/Users/tonio/Desktop/pds-threaded-server/esp.txt"

MyServer::MyServer(QObject *parent):
    QTcpServer (parent),
    connectedClients(0),
    totClients(1),
    currMinute(0),
    DBinitialized(false),
    firstStart(true),
    elabTimerTimeout(false),
    startCalled(false){

    try {
        espMap = new QMap<QString, Esp>();
        mutex = new QMutex();
    //    listenerThreadList = new QList<ListenerThread*>();
        packetsMap = new QMap<QString, Packet>;
        packetsDetectionMap = new QMap<QString, int>;
        peopleMap = new QMap<QString, Person>;
        devicesCoords = new QList<QPointF>;

        connect(&startTimer, &QTimer::timeout, this, &MyServer::startToClientsSlot);
        connect(&elaborateTimer, &QTimer::timeout, this, &MyServer::createElaborateThread);
    } catch (bad_alloc e) {
        fatalErrorSig("Errore nell'acquisizione delle risorse del server");
        exit(-3);
    }

}

//void MyServer::timeout(){

//    // todo: distruggere i thread che non hanno mandato end packet
//    for(auto t : listenerThreadPool.values()){
//        if(!t->endPacketSent)
//            emit closeConnectionSig();
//    }
//    createElaborateThread();
//}

/**
 * @brief MyServer::setMaxEspCoords
 * cerca x e y max tra le coordinate degli esp
 * @param espMap, mappa degli esp
 * @return QPoint con le coord max
 */
QPointF MyServer::setMaxEspCoords(QMap<QString, Esp> *espMap) {
    double x_max = 0, y_max = 0;
    QPointF pos;
    for(QMap<QString, Esp>::iterator i=espMap->begin(); i!=espMap->end(); i++) {
        double x = i.value().getPosition().x();
        double y = i.value().getPosition().y();
        if (x > x_max)
            x_max = x;
        if (y > y_max)
            y_max = y;
    }
    pos.setX(x_max);
    pos.setY(y_max);

    return pos;
}

/**
 * @brief MyServer::init
 * legge le posizioni degli esp da file, cerca le loro coord max e inizializza il db
 */
void MyServer::init(){
    firstStart = true;
    confFromFile();
    emit logSig("[ server ] Esp configuration acquired from file");
    maxEspCoords = setMaxEspCoords(espMap);
    if(maxEspCoords.x() > maxEspCoords.y())
        maxSignal = Utility::metersToDb(maxEspCoords.y());
    else
        maxSignal = Utility::metersToDb(maxEspCoords.x());
    try {
        QThread *thread = new QThread();
        dbthread = new DBThread(this);
        dbthread->moveToThread(thread);
        dbthread->signalsConnection(thread);
        thread->start();
    } catch (bad_alloc e) {
        fatalErrorSig("Errore nell'inizializzazione delle risorse per il dbThread (bad_alloc)");
        exit(-3);
    } catch (exception e) {
        fatalErrorSig("Errore nella fase di inizializzazione del dbThread: " + QString(e.what()));
    }
}

void MyServer::dbConnectedSlot(){
    emit dbConnectedSig();
}

//void MyServer::drawMapSlot(){
//    emit drawMapSig(devicesCoords, maxEspCoords);
//}

/**
 * @brief MyServer::incomingConnection
 * genera un listenerThread all'arrivo di una nuova connessione
 * @param socketDescriptor: descrittore del socket in ascolto
 */
void MyServer::incomingConnection(qintptr socketDescriptor){
    try {
        QThread *thread = new QThread();
        ListenerThread *lt = new ListenerThread(this, socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap, maxSignal, totClients);

        lt->moveToThread(thread);
        lt->signalsConnection(thread);

        thread->start();
    } catch (exception e) {
        emit logSig("Errore nell'inizializzazione di un listener thread: " + QString(e.what()));
    }
}

/**
 * @brief MyServer::addListenerThreadSlot -> slot chiamato dal listenerthread
 * cancella il vecchio thread con stesso id, se presente, e aggiunge quello nuovo
 * @param lt -> nuovo thread da aggiungere
 */
void MyServer::addListenerThreadSlot(ListenerThread* lt){
    QString newId = lt->getId();
    auto it = listenerThreadPool.find(newId);

    if( it != listenerThreadPool.end()){
        listenerThreadPool.erase(it);
        connectedClients--;
    }

    connectedClients++;
    listenerThreadPool.insert(newId, lt);
}

/**
 * @brief MyServer::createElaborateThread
 * -> controlla se ha ricevuto il segnale di "pacchetti ricevuti" da tutti i listenerThread
 * -> genera l'elaborateThread per ottenere i device nell'area nel minuto passato
 * -> manda segnale di start per il nuovo minuto
 */
void MyServer::createElaborateThreadSlot(){
    // todo: lock

//    if(!startCalled)
        // todo: unlock
    while(!startCalled){
    }
    // todo: lock

    // todo: controllare che l'esp che ha mandato l'end packet non sia disconnesso
    endPkClients ++;

    // se ultimo minuto e arrivati almeno 2 end packet fai partire i nuovi 5 minuti
    if(currMinute>5 && endPkClients==2){
        currMinute = 0;
        startToClientsSlot();
    }

    if(endPkClients==connectedClients){
        qDebug() << "createElaborateThreadSlot, currMinute = " << currMinute << ", endPkClients: " << endPkClients;
        if(elaborateTimer.isActive()){
            elaborateTimer.stop();
            createElaborateThread();
        }
    }
}

void MyServer::createElaborateThread(){
    endPkClients = 0;
    startCalled = false;

    try {
        QThread *thread = new QThread();
        ElaborateThread *et = new ElaborateThread(this, packetsMap, packetsDetectionMap, mutex, connectedClients,
                                                  peopleMap, currMinute, espMap, maxEspCoords, devicesCoords); 
        qDebug() << "[server] ----------> new elab thread creato";
        et->moveToThread(thread);
        et->signalsConnection(thread);
        thread->start();
    } catch (bad_alloc e) {
        emit logSig("exception in allocating Elaborate thread, skipping elaboration of current minute.");
        return;
    } catch(exception e) {
        emit logSig(e.what());
    }
}

void MyServer::emitLogSlot(QString message){
    emit logSig(message);
}

/**
 * @brief MyServer::readyFromClient
 * -> slot che risponde al segnale di completamento del setup di un esp
 * -> se ricevuto da tutti gli esp, manda segnale di start del blocco di N minuti
 */
void MyServer::readyFromClientSlot(ListenerThread *lt){

    //inserisco nuovo thread nella lista e aggiorno connectedClients
    addListenerThreadSlot(lt);

    QString s = "Connected clients: ";
    s += QString::number(connectedClients);
    logSig(s);

    if(connectedClients==totClients
//            && Utility::canTriangulate(connectedClients)
            && firstStart
            ){
        firstStart = false;
        currMinute = 0;
        // inizio blocco N minuti
        qDebug() << "readyFromClientSlot(), currMinute = 0";
        startToClientsSlot();
    }
    else{
        //todo: ?? in teoria niente
    }

}

/**
 * @brief MyServer::startToClients
 * manda start agli esp se:
    -> inizialmente tutti i client sono connessi
    -> le volte successive se almeno 3 client connessi
 */
void MyServer::startToClientsSlot(){
    if(connectedClients >= 2){
        qDebug() << "Start to clients\n";
        currMinute++;
        startCalled = true;
        if(currMinute != 1)
            elaborateTimer.start(MyServer::elaborateTime);

        if(currMinute > 5)
            return;
        emit logSig("\n[server] Current minute " + QString::number(currMinute) + ": sending start...\n");        
        emit start2ClientsSig();
        startTimer.start(MyServer::intervalTime);

    }
    else {
        // todo: meno di 2 client connessi => ?????
        // paccio partire timer di reboot
    }
}

void MyServer::disconnectClientSlot(QString espId){
    auto it = listenerThreadPool.find(espId);

    if(it!=listenerThreadPool.end())
        listenerThreadPool.erase(it);

    connectedClients--;
    QString s = "Client ";
    s += espId;
    s += " disconnected";
    logSig(s);
}

void MyServer::errorFromThreadSlot(QString errorMsg){
    emit fatalErrorSig(errorMsg);
    return;
}

/**
 * @brief MyServer::confFromFile
 * legge posizioni degli esp da file
 */
void MyServer::confFromFile(){
    ifstream inputFile;
    int i;
    string id, mac, x_str, y_str;
    QString qmac;
    double x_double, y_double;
    int retry = 3;

    while(retry) {
        try {
            inputFile.open(ESP_FILE_PATH);
            inputFile >> totClients;
            for(i=0; i< totClients; i++){
                inputFile >> id >> mac >> x_str >> y_str;
                x_double = stod(x_str.c_str());
                y_double = stod(y_str.c_str());
                QPointF point = QPointF(x_double, y_double);
                qmac = QString::fromStdString(mac);
                const Esp esp = Esp(QString::fromStdString(id), qmac, point);
                espMap->insert(qmac, esp);
            }
            inputFile.close();
            retry = 0;
        } catch (...) {
            retry--;
            if (!retry) {
                qDebug() << "Errore apertura file";
                emit fatalErrorSig("Impossibile aprire il file degli esp");
                this->~MyServer();
                exit(-2);
            }
        }
    }


}

/**
 * @brief MyServer::dataForDb
 */
void MyServer::onChartDataReadySlot(){
    mutex->lock();
    // pulisce le strutture dati per il time slot successivo
    packetsMap->clear();
//    packetsMap = new QMap<QString, Packet>(); // todo: da rivedere
    packetsDetectionMap->clear();

    //emetto segnale verso dbthread
    QMap<QString, Person> people = *peopleMap;
    peopleMap->clear();
    emit chartDataToDbSig(people);

    //passo copia dati da disegnare alla mappa e cancello quelli vecchi
    QList<QPointF> coordsForMap = *devicesCoords;
    emit drawMapSig(coordsForMap, maxEspCoords);
    devicesCoords->clear();
    mutex->unlock();
}

/**
 * @brief MyServer::emitDrawRunTimeChartSignalSlot
 * emette il segnale per far sì che la mainwindow disegni il grafico runtime del conto delle persone nell'area
 * Avendo messo una funzione apposita solo per emettere il segnale, non ho creato dipendenze tra dbthrad e myserver
 */
void MyServer::emitDrawChartSlot(QMap<QString, int> chartDataToDrawMap) {
    emit drawChartSig(chartDataToDrawMap);
}

/**
 * @brief MyServer::DrawOldCountMap
 * @param begintime
 * @param endtime
 * @return
 */
//QList<QPointF> *MyServer::DrawOldCountMap(QString begintime, QString endtime) {
//    //da sistemare
//    QList<QPointF> *peopleCounter = new QList<QPointF>();
//    emit getStatsSig(begintime, endtime);

//    //controlla
//    return peopleCounter;


//}

//void MyServer::Connects(QString slot) {
//    QThread *thread = new QThread();
//    DBThread *dbthread = new DBThread(peopleMap, devicesCoords->size(), DBinitialized);
//    dbthread->moveToThread(thread);
//    connect(dbthread, SIGNAL(finished()), thread, SLOT(quit()));
//    connect(dbthread, SIGNAL(finished()), dbthread, SLOT(deleteLater()));
//    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
//}

MyServer::~MyServer() {
    qDebug() << "Distruttore MyServer";
    delete espMap;
    delete mutex;
    delete packetsMap;
    delete packetsDetectionMap;
    delete peopleMap;
    delete devicesCoords;
}
