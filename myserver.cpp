#include "mainwindow.h"
#include "myserver.h"
#include "utility.h"
#include <fstream>
#include <memory>
using namespace std;

//#define ESP_FILE_PATH "C:/Users/raffy/Desktop/PDS_09-09/pds-threaded-server/esp.txt"
#define ESP_FILE_PATH "C:/Users/tonio/Desktop/pds-threaded-server/esp.txt"

MyServer::MyServer(QObject *parent):
    QTcpServer (parent),    
    connectedClients(0),    
    totClients(1),
    currMinute(0),
    DBinitialized(false),
    firstStart(true),    
    elabTimerTimeout(false),
    tag("MyServer"),
    previousMinute(0){

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
        return;
    }

}

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
    emit logSig("- Esp configuration acquired from file", "green");
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
        emit drawMapSig(QList<QPointF>(), maxEspCoords, QMap<QString, Person>(), espMap);
    } catch (bad_alloc e) {
        fatalErrorSig("Errore nell'inizializzazione delle risorse per il dbThread (bad_alloc)");
        return;
    } catch (exception e) {
        fatalErrorSig("Errore nella fase di inizializzazione del dbThread: " + QString(e.what()));
        return;
    }
}

void MyServer::dbConnectedSlot(){
    emit dbConnectedSig();
}

/**
 * @brief MyServer::incomingConnection
 * genera un listenerThread all'arrivo di una nuova connessione
 * @param socketDescriptor: descrittore del socket in ascolto
 */
void MyServer::incomingConnection(qintptr socketDescriptor){
    try {
        QThread *thread = new QThread();
        ListenerThread *lt = new ListenerThread(this, socketDescriptor, mutex, packetsMap, packetsDetectionMap, espMap, maxSignal, totClients, &currMinute);

        lt->moveToThread(thread);
        lt->signalsConnection(thread);

        thread->start();
    } catch (exception e) {
        emit logSig(tag + ": Errore nell'inizializzazione di un listener thread: " + QString(e.what()), "red");
    }
}

/**
 * @brief MyServer::addListenerThreadSlot -> slot chiamato dal listenerthread
 * cancella il vecchio thread con stesso id, se presente, e aggiunge quello nuovo
 * @param lt -> nuovo thread da aggiungere
 */
void MyServer::addListenerThreadSlot(ListenerThread* lt){
    mutex->lock();
    QString newId = lt->getId();
    QString mac = lt->getMac();
    auto it = listenerThreadPool.find(newId);

    if( it != listenerThreadPool.end()){
        listenerThreadPool.erase(it);
        connectedClients--;
        emit logSig("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", "black");
        emit logSig("Reconnection", "orange");
        emit logSig("client info: id = " + newId + ", mac = " + mac, "black");
    }
    else{
        emit logSig("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", "black");
        emit logSig("New connection", "orange");
        emit logSig("client info: id = " + newId + ", mac = " + mac, "black");
    }

    connectedClients++;
    listenerThreadPool.insert(newId, lt);
    emit setClientsSig(connectedClients);
    mutex->unlock();
}

/**
 * @brief MyServer::createElaborateThread
 * -> controlla se ha ricevuto il segnale di "pacchetti ricevuti" da tutti i listenerThread
 * -> genera l'elaborateThread per ottenere i device nell'area nel minuto passato
 * -> se minuto 5 allora manda segnale di start per il nuovo minuto
 */
void MyServer::createElaborateThreadSlot(){
    mutex->lock();

    endPkClients ++;

    if(endPkClients==connectedClients){
        mutex->unlock();
        createElaborateThread();
    }
    else
        mutex->unlock();
}

void MyServer::restart(){
    mutex->lock();
    connectedClients = 0;
    emit setClientsSig(connectedClients);
    firstStart = true;
    mutex->unlock();
}

void MyServer::setMinuteSlot(int minute){
    emit setMinuteSig(minute);
}

void MyServer::createElaborateThread(){
    try {
        mutex->lock();
        endPkClients = 0;

        elaborateTimer.stop();
        qDebug() << tag << ": createElaborateThreadSlot, currMinute = " << currMinute-1 << ", endPkClients: " << endPkClients;

        QThread *thread = new QThread();
        ElaborateThread *et = new ElaborateThread(this, packetsMap, packetsDetectionMap, mutex, connectedClients,
                                                  peopleMap, currMinute-1, espMap, maxEspCoords, devicesCoords);
        et->moveToThread(thread);
        et->signalsConnection(thread);
        thread->start();

        emit logSig("Elaboration minute " +  QString::number(currMinute-1), "orange");
        emit logSig("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", "black");
        if((currMinute-1)<MAX_MINUTES){
            emit logSig("Minute " + QString::number(currMinute) +  ": waiting packets...", "orange");
            mutex->unlock();
        }
        else{
            if(connectedClients!=0){
                emit logSig("Minute 1 : waiting packets...", "orange");
                currMinute = 0;
                mutex->unlock();
                startToClientsSlot();
            }
            else{
                mutex->unlock();
                restart();
            }
        }
    } catch (bad_alloc e) {
        emit logSig(tag + ": Exception in allocating Elaborate thread, skipping elaboration of current minute", "orange");
        mutex->unlock();
    } catch(exception e) {
        emit logSig(tag + ": error in elaboration thread, skipping current minute", "orange");
        mutex->unlock();
    }
}

void MyServer::emitLogSlot(QString message, QString color){
    emit logSig(message, color);
}

/**
 * @brief MyServer::readyFromClient
 * -> slot che risponde al segnale di completamento del setup di un esp
 * -> se ricevuto da tutti gli esp, manda segnale di start del blocco di N minuti
 */
void MyServer::readyFromClientSlot(ListenerThread *lt){
    //inserisco nuovo thread nella lista e aggiorno connectedClients
    addListenerThreadSlot(lt);

    mutex->lock();
    if(connectedClients==totClients && firstStart){
        emit logSig("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------", "black");
        emit logSig("Start to clients...", "green");
        emit logSig("Minute 1 : waiting packets...", "orange");
        currMinute = 0;
        endPkClients = 0;
        mutex->unlock();
        // inizio blocco N minuti
        startToClientsSlot();
    }
    else
        mutex->unlock();
}

/**
 * @brief MyServer::startToClients
 * manda start agli esp se:
    -> inizialmente tutti i client sono connessi
    -> le volte successive se almeno 3 client connessi
 */
void MyServer::startToClientsSlot(){
    mutex->lock();

    currMinute++;

    if(!firstStart){
        elaborateTimer.start(MyServer::elaborateTime);
    }
    else{
        firstStart = false;
    }

    if(currMinute > MAX_MINUTES){
        mutex->unlock();
        return;
    }

    emit setMinuteSig(currMinute);
    qDebug() << tag + ": startToClientsSlot, minute=" + QString::number(currMinute);

    emit start2ClientsSig(currMinute);
    startTimer.start(MyServer::intervalTime);

    mutex->unlock();
}

void MyServer::disconnectClientSlot(QString espId){
//    mutex->lock();
    auto it = listenerThreadPool.find(espId);

    if(it!=listenerThreadPool.end()){
        listenerThreadPool.erase(it);
        closeConnectionSig(espId);
        connectedClients--;
        emit setClientsSig(connectedClients);
        QString s = "Client ";
        s += espId;
        s += " disconnected";
        logSig(s, "orange");
    }
//    mutex->unlock();
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
                qDebug() << tag << ": Errore apertura file";
                emit fatalErrorSig(tag + ": Impossibile aprire il file degli esp");
            }
        }
    }


}

/**
 * @brief MyServer::onChartDataReadySlot
 * richiamata dopo l'elaborazione del quinto minuto: pulisce strutture, passa dati al db e alla gui
 */
void MyServer::onChartDataReadySlot(){
    try {
        mutex->lock();
        // pulisce le strutture dati per il time slot successivo
        packetsMap->clear();
        packetsDetectionMap->clear();

        //emetto segnale verso dbthread
        QMap<QString, Person> people = *peopleMap;
        peopleMap->clear();
        emit chartDataToDbSig(people);

        //passo copia dati da disegnare alla mappa e cancello quelli vecchi
        QList<QPointF> coordsForMap = *devicesCoords;
        emit drawMapSig(coordsForMap, maxEspCoords, people, espMap);
        devicesCoords->clear();
        mutex->unlock();
    } catch (...) {
        mutex->unlock();
    }

}

void MyServer::getMinDateForLPSTATSSlot() {
    emit getMinDateForLPStatsSig();
}

void MyServer::LPStatsWindowCreationSlot(QString minDate, QString maxDate) {
    emit LPStatsWindowCreationSig(minDate, maxDate);
}

void MyServer::getLPStatsSlot(QString begintime, QString endtime) {
    emit getLPStatsSig(begintime, endtime);
}

void MyServer::LPStatsSlot(QMap<QString, QList<QString>> map) {
    emit LPStatsSig(map);
}

/**
 * @brief MyServer::emitDrawRunTimeChartSignalSlot
 * emette il segnale per far sì che la mainwindow disegni il grafico runtime del conto delle persone nell'area
 * Avendo messo una funzione apposita solo per emettere il segnale, non ho creato dipendenze tra dbthrad e myserver
 */
void MyServer::emitDrawChartSlot(QMap<QString, int> chartDataToDrawMap) {
    emit drawChartSig(chartDataToDrawMap);
}


MyServer::~MyServer() {
    mutex->lock();
    qDebug() << tag << ": Distruttore MyServer";

    for(auto i : listenerThreadPool){
        emit closeConnectionSig(i->getId());
    }

    delete espMap;
    espMap = nullptr;
    delete packetsMap;
    packetsMap = nullptr;
    delete packetsDetectionMap;
    packetsDetectionMap = nullptr;
    delete peopleMap;
    peopleMap = nullptr;
    delete devicesCoords;
    devicesCoords = nullptr;

    mutex->unlock();
    delete mutex;
    mutex = nullptr;
}
