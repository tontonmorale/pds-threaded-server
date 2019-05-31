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

QSqlDatabase DBThread::db;

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

    QSqlQuery query;
    QString queryString;

    //query per creazione tabella timestamp
    queryString = "CREATE TABLE IF NOT EXISTS Timestamps ("
            "timestamp VARCHAR(255) NOT NULL, "
            "count INT, "
            "PRIMARY KEY (timestamp)"
            ") ENGINE = InnoDB;";


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
            ") ENGINE = InnoDB;";

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
    connect(this, &DBThread::logSig, server, &MyServer::emitLogSlot);
    connect(this, &DBThread::finishedSig, server, &MyServer::clearPeopleMapSlot);
    connect(this, &DBThread::drawRuntimeChartSig, server, &MyServer::emitDrawRuntimeChartSignalSlot);
    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}

void DBThread::send(QMap<QString, Person> *peopleMap, int size)
{
    this->peopleMap=peopleMap;
    this->size = size;
    QSqlQuery query;
    QString queryString;

    qDebug().noquote() << "send()";

    //inserisco la nuova entry timestamp-numero di persone rilevate a quel timestamp nella tabella timestamp
    if (!dbConnect()) {
        qDebug().noquote() << "send(): dbConnect fallita.";
        return;
    }
    qDebug().noquote() << "send(): dbConnect andata a buon fine.";
    QMap<QString, Person>::iterator pm = this->peopleMap->begin();
    QSet<QSharedPointer<Packet>> ps = pm.value().getPacketsSet();
    QString timestamp = (*ps.begin())->getTimestamp();
    QStringList tsSplit = timestamp.split(':');
    //Il timestamp ricevuto è del tipo: 22/03/2019_17:52:14
    //in questa maniera vado a memorizzare l'intera stringa tranne i secondi
    QString Timestamp = tsSplit.at(0) + tsSplit.at(1);
    qDebug().noquote() << "Timestamp delle persone nella peopleMap: " + Timestamp;

    queryString = "INSERT INTO Timestamps (timestamp, count) "
                              "VALUES ('" + Timestamp + "', " + QString::number(this->size) + ");";
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
        queryString += "('" + Timestamp + "', '" + pm.key() + "'),";
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
    emit finishedSig();
    }

void DBThread::dbDisconnect(){
    if(db.isOpen())
        db.close();
}

bool DBThread::dbConnect() {
    this->db = QSqlDatabase::addDatabase("QMYSQL");
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
void DBThread::GetTimestampsFromDB(QList<QPointF> *peopleCounter, QString begintime, QString endtime) {
    this->peopleCounter = peopleCounter;
    QSqlQuery query;
    QString queryString;
    QMap<QString, int> oldCountMap;

    queryString = "SELECT * "
                  "FROM Timestamps "
                  "WHERE timestamp > " + begintime + " AND timestamp < " + endtime + ";";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        int i = 0, j = 1;
        //popolo la mappa delle persone
        while (query.next()) {
            oldCountMap.insert(query.value(i).toString(), query.value(j).toInt());
            i += 2;
            j += 2;
        }
        //setto la lista di punti con persone ricevute dal db
        QPointF point;
        for (QMap<QString, int>::iterator i = oldCountMap.begin(); i != oldCountMap.end(); i++) {
            point = QPointF(i.key().toInt(), i.value()); //Verifica che funziona la conversione da timestamp stringa a timestamp intero
            peopleCounter->append(point);
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
    QSqlQuery query;
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








