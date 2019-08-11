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

DBThread::DBThread(MyServer* server) :
    server(server) {

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

    // dati per il primo disegno del chart
    QString begintime, endtime;
    QDateTime curr_timestamp = QDateTime::currentDateTime();
    endtime = curr_timestamp.toString("yyyy/MM/dd_hh:mm");
    QDateTime old_timestamp(QDate(curr_timestamp.date()), QTime(curr_timestamp.time().hour()-1, curr_timestamp.time().minute()));
    begintime = old_timestamp.toString("yyyy/MM/dd_hh:mm");
    getChartDataFromDb(begintime, endtime);

    emit logSig("[ db thread ] Db connection established");
    emit dbConnectedSig();

    return;
}

void DBThread::signalsConnection(QThread *thread){

    qDebug().noquote() << "signalsConnection()";

    qRegisterMetaType<QMap<QString, int>>("QMap<QString, int>");
    qRegisterMetaType<QMap<QString, Person>>("QMap<QString, Person>");

    connect(thread, SIGNAL(started()), this, SLOT(run()));

    connect(server, &MyServer::chartDataToDbSig, this, &DBThread::sendChartDataToDbSlot);
    connect(server, &MyServer::getStatsSig, this, &DBThread::GetLPSFromDB);

    connect(this, &DBThread::logSig, server, &MyServer::emitLogSlot);
    connect(this, &DBThread::drawChartSig, server, &MyServer::emitDrawChartSlot);
    connect(this, SIGNAL(finished()), thread, SLOT(quit()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, &DBThread::fatalErrorSig, server, &MyServer::errorFromThreadSlot);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(this, &DBThread::dbConnectedSig, server, &MyServer::dbConnectedSlot);
}

void DBThread::sendChartDataToDbSlot(QMap<QString, Person> peopleMap)
{
    int size = peopleMap.size();
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    qDebug().noquote() << "send()";

    //inserisco la nuova entry timestamp-numero di persone rilevate a quel timestamp nella tabella timestamp
    if (!isDbOpen()) {
        qDebug().noquote() << "send(): db not open";
        emit fatalErrorSig("Db is not open");
        return;
    }

    // calcola timestamp unico per lo slot attuale di 5 minuti
    QDateTime timestampDT = calculateTimestamp();
    QString timestamp = timestampDT.toString("yyyy/MM/dd_hh:mm");

    //Il timestamp ricevuto è del tipo: 2019/03/22_17:52:14
    //in questa maniera vado a memorizzare l'intera stringa tranne i secondi
    qDebug().noquote() << "Timestamp delle persone nella peopleMap: " + timestamp;

    queryString = "INSERT INTO Timestamps (timestamp, count) "
                              "VALUES ('" + timestamp + "', " + QString::number(size) + ");";
    if (DBThread::db.open())
        qDebug().noquote() << "il db è open.";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        qDebug() << "Query di inserzione della nuova entry nella tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di inserzione della nuova entry nella tabella timestamp fallita";
        emit logSig("Cannot update data on db");
    }

    if(peopleMap.size()!=0){
        //inserisco le persone rilevate nell'altra tabella
        //nota: vado a considerare il timestamp preso prima come timestamp generico per tutte le persone presenti nella peopleMap attuale
        queryString = "INSERT INTO LPStats (timestamp, mac) VALUES ";
        for(auto pm=peopleMap.begin(); pm!=peopleMap.end(); pm++){
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
    }

    QString begintime;
    QDateTime old_timestamp(QDate(timestampDT.date()), QTime(timestampDT.time().hour()-1, timestampDT.time().minute()));
    begintime = old_timestamp.toString("yyyy/MM/dd_hh:mm");
    getChartDataFromDb(begintime, timestamp);
}

QDateTime DBThread::calculateTimestamp(){
    QDateTime timestamp = QDateTime::currentDateTime();
    begintime = timestamp.toString("yyyy/MM/dd_hh:mm");
    int minute = timestamp.toString("mm").toInt();
    QDateTime correct_timestamp;
    if (int resto = (minute % 5 != 0)){
        correct_timestamp = QDateTime(QDate(timestamp.date()), QTime(timestamp.time().hour(), timestamp.time().minute()-resto));
        return correct_timestamp;
    }
    else {
        return timestamp;
    }
}

void DBThread::dbDisconnect(){
    if(db.isOpen())
        db.close();
    emit finished();
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
void DBThread::getChartDataFromDb(QString begintime, QString endtime) {
    QMap<QString, int> chartDataToDrawMap;
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    queryString = "SELECT * "
                  "FROM Timestamps "
                  "WHERE timestamp > '" + begintime + "' AND timestamp <= '" + endtime + "';";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        //popolo la mappa delle persone
        qDebug() << "Query di select dalla tabella Timestamps good";
        while (query.next()) {
            chartDataToDrawMap.insert(query.value(0).toString(), query.value(1).toInt());
        }
        emit drawChartSig(chartDataToDrawMap);
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

DBThread::~DBThread() {
    qDebug() << "Distruttore DBThread";
    if(db.isOpen())
        db.close();
}








