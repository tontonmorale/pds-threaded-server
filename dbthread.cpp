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
    server(server),
    tag("DBThread") {
}

void DBThread::run() {

    if (!dbConnect()){
        dbDisconnect();
        emit fatalErrorSig(tag + ": Database connection failed");
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


//    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << tag << " : CREATE tabella Timestamps ok";
    }
    else{
        qDebug() << tag << " : CREATE tabella Timestamp fallita";
        dbDisconnect();
        emit fatalErrorSig(tag + ": TIMESTAMPS table creation failed");
        return;
    }

    //LPStats = long period statistics
    // ts = timestamp
    queryString = "CREATE TABLE IF NOT EXISTS LPStats ("
            "timestamp VARCHAR(255) NOT NULL, "
            "mac VARCHAR(255) NOT NULL, "
            "PRIMARY KEY (timestamp, mac)"
            ");";

//    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << tag << " : Create timestamp ok";
    }
    else{
        qDebug() << tag << " : Create tabella LPStats fallita";
        dbDisconnect();
        emit fatalErrorSig(tag + ": LPSTATS table creation failed");
        return;
    }

    // dati per il primo disegno del chart
    QString begintime, endtime;
    QDateTime curr_timestamp = QDateTime::currentDateTime();
    endtime = curr_timestamp.toString("yyyy/MM/dd_hh:mm");
    QTime t;
    if(curr_timestamp.time().minute()>=12)
        t = QTime(curr_timestamp.time().hour(), curr_timestamp.time().minute()-12);
    else
        t = QTime(curr_timestamp.time().hour()-1, 60+(curr_timestamp.time().minute()-12));

    QDateTime old_timestamp(QDate(curr_timestamp.date()), t);
    begintime = old_timestamp.toString("yyyy/MM/dd_hh:mm");
    getChartDataFromDb(begintime, endtime);

    emit logSig(tag + ": Db connection established");
    emit dbConnectedSig();

    return;
}

void DBThread::signalsConnection(QThread *thread){

    qDebug().noquote() << tag << " : signalsConnection()";

    qRegisterMetaType<QMap<QString, int>>("QMap<QString, int>");
    qRegisterMetaType<QMap<QString, Person>>("QMap<QString, Person>");
    qRegisterMetaType<QMap<QString, QList<QString>>>("QMap<QString, QList<QString>>");

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
    connect(server, &MyServer::getMinDateForLPStatsSig, this, &DBThread::getMinDateForLPSTATSSlot);
    connect(this, &DBThread::LPStatsWindowCreationSig, server, &MyServer::LPStatsWindowCreationSlot);
    connect(server, &MyServer::getLPStatsSig, this, &DBThread::GetLPSFromDB);
    connect(this, &DBThread::LPStatsSig, server, &MyServer::LPStatsSlot);

}

void DBThread::sendChartDataToDbSlot(QMap<QString, Person> peopleMap)
{
    int size = peopleMap.size();
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    qDebug().noquote() << tag << " : sendChartDataToDbSlot()";

    //inserisco la nuova entry timestamp-numero di persone rilevate a quel timestamp nella tabella timestamp
    if (!isDbOpen()) {
        qDebug().noquote() << tag << " : sendChartDataToDbSlot(): db not open";
        emit fatalErrorSig(tag + ": Db is not open");
        return;
    }

    // calcola timestamp unico per lo slot attuale di 5 minuti
    QDateTime timestampDT = calculateTimestamp();
    QString timestamp = timestampDT.toString("yyyy/MM/dd_hh:mm");

    //Il timestamp ricevuto è del tipo: 2019/03/22_17:52:14
    //in questa maniera vado a memorizzare l'intera stringa tranne i secondi
    qDebug().noquote() << tag << " : Timestamp delle persone nella peopleMap: " + timestamp;

    queryString = "INSERT INTO Timestamps (timestamp, count) "
                              "VALUES ('" + timestamp + "', " + QString::number(size) + ");";
    if (DBThread::db.open())
        qDebug().noquote() << tag << " : il db è open";

//    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        qDebug() << tag << " : INSERT in Timestamp ok";
    }
    else{
        qDebug() << tag << " : INSERT in Timestamp fallita";
        emit logSig(tag + ": INSERT in tabella Timestamp fallita");
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

//        qDebug().noquote() << "query: " + queryString;
        if (query.exec(queryString)) {
            qDebug() << tag << " : Query di inserzione delle persone nella tabella LPStats andata a buon fine";
        }
        else{
            qDebug() << "Query di inserzione delle persone nella tabella LPStats fallita";
            return;
        }
    }

    QString begintime;
    QTime t;
    if(timestampDT.time().minute()>=12)
        t = QTime(timestampDT.time().hour(), timestampDT.time().minute()-12);
    else
        t = QTime(timestampDT.time().hour()-1, 60+(timestampDT.time().minute()-12));
    QDateTime old_timestamp(QDate(timestampDT.date()), t);
    begintime = old_timestamp.toString("yyyy/MM/dd_hh:mm");
    getChartDataFromDb(begintime, timestamp);
}

QDateTime DBThread::calculateTimestamp(){
    QDateTime timestamp = QDateTime::currentDateTime();

    return timestamp;
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
        qDebug().noquote() << tag << " : Connection to db extablished\n";
        return true;
    }
    else{
        qDebug().noquote() << tag << " : Impossible to connect to db\n";
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

//    qDebug().noquote() << tag << " : query: " + queryString;
    if (query.exec(queryString)) {
        //popolo la mappa delle persone
        qDebug() << tag << " : Query di select dalla tabella Timestamps good";
        while (query.next()) {
            chartDataToDrawMap.insert(query.value(0).toString(), query.value(1).toInt());
        }

        //se un solo dato => ne aggiungo uno prima, con y a 0
        if(chartDataToDrawMap.size()==1){
            const QString& date_string = chartDataToDrawMap.firstKey();
            QDateTime date_dt = QDateTime::fromString(date_string,"yyyy/MM/dd_hh:mm");
            QDateTime newDate_dt = date_dt.addSecs(-60);
            QString newDate_str = newDate_dt.toString("yyyy/MM/dd_hh:mm");
            chartDataToDrawMap.insert(newDate_str, 0);
        }
        emit drawChartSig(chartDataToDrawMap);
        return;

    }
    else{
        qDebug() << tag << " : Query di select dalla tabella Timestamps fallita";
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

void DBThread::getMinDateForLPSTATSSlot() {
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString;

    queryString = "SELECT MIN(timestamp), MAX(timestamp) "
                  "FROM LPStats;";
    if (query.exec(queryString)) {
        if (query.next()){
            QString min = query.value(0).toString(), max = query.value(1).toString();
            emit LPStatsWindowCreationSig(min, max);
        }
        else emit LPStatsWindowCreationSig("", "");
    }
    else {
        return emit LPStatsWindowCreationSig("", "");
    }
}

void DBThread::GetLPSFromDB(QString begintime, QString endtime) {
    QSqlQuery query(QSqlDatabase::database("connection"));
    QString queryString, queryString2;
    QList<QString> macList;
    QMap<QString, QList<QString>> map;

    queryString = "SELECT * "
                  "FROM (SELECT mac, COUNT(mac) as counter "
                   "FROM LPStats "
                   "WHERE timestamp >= '" + begintime + "' AND timestamp <= '" + endtime + "' "
                   "GROUP BY mac) as Tabella "
                   "ORDER BY Tabella.counter DESC "
                   "LIMIT 3;";

    qDebug().noquote() << "query: " + queryString;
    if (query.exec(queryString)) {
        while(query.next()) {
            macList.append(query.value(0).toString());
        }
        for (auto mac : macList){
            queryString2 = "SELECT timestamp "
                           "FROM LPStats "
                           "WHERE timestamp >= '" + begintime + "' AND timestamp <= '" + endtime + "' "
                           "AND mac = '" + mac + "' "
                           "ORDER BY timestamp;";
            qDebug().noquote() << "query: " + queryString2;
            if (query.exec(queryString2)) {
                while (query.next()) {
                    if (map.find(mac)==map.end()){
                        QList<QString> list;
                        map.insert(mac, list);
                    }
                        map.find(mac)->append(query.value(0).toString());
                }
            }
            else {
                qDebug() << "Query per LPStats fallita "  + query.lastError().text();
                return;
            }
        }
        emit LPStatsSig(map);
    }
    else{
        qDebug() << "Query per i timestamp di LPStats fallita: " + query.lastError().text();
        return;
    }
}

DBThread::~DBThread() {
    qDebug() << "Distruttore DBThread";
    if(db.isOpen())
        db.close();
}








