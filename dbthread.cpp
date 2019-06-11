#include "dbthread.h"
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include "esp.h"
#include "packet.h"
#include <QMutex>
#include <QThread>
#include "myserver.h"

DBThread::DBThread()
{

}

DBThread::DBThread(MyServer* server)
{
    this->server = server;
}

void DBThread::run() {

    if (!dbConnect()){
        dbDisconnect();
        emit fatalErrorSig("Database connection failed");
        return;
    }

    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    //query per creazione tabella timestamp
    queryString = "CREATE TABLE IF NOT EXISTS Timestamps ("
            "timestamp VARCHAR(255) NOT NULL, "
            "count INT, "
            "PRIMARY KEY (timestamp)"
            ");";


    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << "Query di creazione tabella Timestamps andata a buon fine";
    }
    else{
        qDebug() << "Query di creazione tabella timestamp fallita";
        dbDisconnect();
        emit fatalErrorSig("TIMESTAMPS table creation failed");
        return;
    }

    //LPStats = long period statistics
    // ts = timestamp
    queryString = "CREATE TABLE IF NOT EXISTS LPStats ("
            "timestamp VARCHAR(255) NOT NULL, "
            "mac VARCHAR(255) NOT NULL, "
            "PRIMARY KEY (ts, mac)"
            ");";

    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << "Query di creazione tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di creazione tabella timestamp fallita";
        dbDisconnect();
        emit fatalErrorSig("LPSTATS table creation failed");
        return;
    }

    return;
}

void DBThread::signalsConnection(QThread *thread){

    qDebug().noquote() << "signalsConnection()";

    connect(thread, SIGNAL(started()), this, SLOT(run()));

    connect(server, &MyServer::sendToDBSig, this, &DBThread::sendSlot);
    connect(server, &MyServer::getTimestampsSig, this, &DBThread::GetTimestampsFromDB);
    connect(server, &MyServer::getStatsSig, this, &DBThread::GetLPSFromDB);

    connect(this, &DBThread::logSig, server, &MyServer::emitLogSlot);
    connect(this, &DBThread::sendFinishedSig, server, &MyServer::clearPeopleMapSlot);
    connect(this, &DBThread::drawRuntimeChartSig, server, &MyServer::emitDrawRuntimeChartSignalSlot);
    connect(this, SIGNAL(finishedSig()), thread, SLOT(quit()));
    connect(this, SIGNAL(finishedSig()), this, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

void DBThread::sendSlot(QMap<QString, Person> *peopleMap, int size)
{
    this->peopleMap=peopleMap;
    this->size = size;
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    qDebug().noquote() << "send()";

    //inserisco la nuova entry timestamp-numero di persone rilevate a quel timestamp nella tabella timestamp
    if (!isDbOpen()) {
        qDebug().noquote() << "send(): db not open";
        return;
    }
    qDebug().noquote() << "send(): dn open";
    QMap<QString, Person>::iterator pm = this->peopleMap->begin();
    QSet<QSharedPointer<Packet>> ps = pm.value().getPacketsSet();
    QString timestamp = (*ps.begin())->getTimestamp();
    //Il timestamp ricevuto è del tipo: 2019/03/22_17:52:14
    //in questa maniera vado a memorizzare l'intera stringa tranne i secondi
    timestamp = timestamp.left(timestamp.length()-3);
    qDebug().noquote() << "Timestamp delle persone nella peopleMap: " + timestamp;

    queryString = "INSERT INTO Timestamps (timestamp, count) "
                              "VALUES ('" + timestamp + "', " + QString::number(this->size) + ");";
    if (DBThread::db.open())
        qDebug().noquote() << "il db è open.";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        qDebug() << "Query di inserzione della nuova entry nella tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di inserzione della nuova entry nella tabella timestamp fallita";
        return;
    }

    //inserisco le persone rilevate nell'altra tabella
    //nota: vado a considerare il timestamp preso prima come timestamp generico per tutte le persone presenti nella peopleMap attuale
    queryString = "INSERT INTO LPStats (timestamp, mac) VALUES ";
    for(pm=this->peopleMap->begin(); pm!=this->peopleMap->end(); pm++){
        queryString += "('" + timestamp + "', '" + pm.key() + "'),";
    }
    //tolgo l'ultima virgola
    queryString = queryString.left(queryString.length()-1); //da verificare
    queryString += ";";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        qDebug() << "Query di inserzione delle persone nella tabella LPStats andata a buon fine";
    }
    else{
        qDebug() << "Query di inserzione delle persone nella tabella LPStats fallita";
        return;
    }
    emit sendFinishedSig();
    }

void DBThread::dbDisconnect(){
    if(db.isOpen())
        db.close();
    emit finishedSig();
}

bool DBThread::isDbOpen(){
    return db.isOpen();
}

bool DBThread::dbConnect() {
    this->db = QSqlDatabase::addDatabase("QMYSQL", "connection");
    this->db.setHostName("localhost");
    this->db.setDatabaseName("databasepds");
    this->db.setUserName("root");
    this->db.setPassword("");

    if (db.open()){
        qDebug().noquote() << "Connection to db extablished\n";
        return true;
    }
    else{
        qDebug().noquote() << "Impossible to connect to db\n";
        return false;
    }
}

//NOTA: assumo che il begintime e l'endtime siano timestamp correttamente costruiti a partire da data e ora prese in input dall'utente
void DBThread::GetTimestampsFromDB(QMap<QString, int> *peopleCounterMap, QString begintime, QString endtime) {
    this->peopleCounterMap = peopleCounterMap;
    QSqlQuery query((QSqlDatabase::database("connection")));
    QString queryString;

    queryString = "SELECT * "
                  "FROM Timestamps "
                  "WHERE timestamp > '" + begintime + "' AND timestamp < '" + endtime + "';";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        //popolo la mappa delle persone
        while (query.next()) {
            peopleCounterMap->insert(query.value(0).toString(), query.value(1).toInt());
        }
        emit drawRuntimeChartSig();
        return;

    }
    else{
        qDebug() << "Query di select dalla tabella Timestamps fallita";
        return;
    }
}

//QList<QPointF> DBThread::drawOldContMap(QList<QPointF> *peopleCounter) {
//    QPointF point;
//    QList<QPointF> peopleCounter;
//    for (QMap<QString, int>::iterator i = oldCountMap->begin(); i != oldCountMap->end(); i++) {
//        point = QPointF(i.key().toInt(), i.value()); //Verifica che funziona la conversione da timestamp stringa a timestamp intero
//        peopleCounter.append(point);
//    }
//    return peopleCounter;
//}

void DBThread::GetLPSFromDB(QString begintime, QString endtime) {
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    queryString = "SELECT mac, COUNT(*) "
                  "FROM LPStats "
                  "WHERE timestamp > " + begintime + " AND timestamp < " + endtime +
                  "GROUP BY mac"
                  "ORDER BY COUNT(*);";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        //popolo la mappa delle persone

        //disegna grafico con persone ricevute dal db
    }
    else{
        qDebug() << "Query di select dalla tabella Timestamps fallita";
        return;
    }
}








