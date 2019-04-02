#include "dbthread.h"

DBThread::DBThread()
{

}

DBThread::DBThread(QMap<QString, Person> *peopleMap)
{
    //crea init e controlla se è gia stata fatta con booleano
    this->peopleMap=peopleMap;

    if (!dbConnect())
        return; //gestione mancata connessione da fare

    QSqlQuery query;
    QString queryString;

    //query per creazione tabella timestamp
    queryString = "CREATE TABLE IF NOT EXISTS timestamps ("
            "timestamp VARCHAR(255) NOT NULL, "
            "count INT, "
            "PRIMARY KEY (timestamp)"
            ") ENGINE = InnoDB;";

    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << "Query di creazione tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di creazione tabella timestamp fallita";
        return;
    }

    //LPStats = long period statistics
    queryString = "CREATE TABLE IF NOT EXISTS LPStats ("
            "timestamp VARCHAR(255) NOT NULL, "
            "mac VARCHAR(255) NOT NULL, "
            "PRIMARY KEY (timestamp, mac)"
            ") ENGINE = InnoDB;";

    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << "Query di creazione tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di creazione tabella timestamp fallita";
        return;
    }

}

void DBThread::send()
{
    QSqlQuery query;
    QString queryString;
/*
    if (!dbConnect())
        return;



    //query per creazione tabella timestamp
    queryString = "CREATE TABLE IF NOT EXISTS timestamps ("
            "timestamp VARCHAR(255) NOT NULL, "
            "count INT, "
            "PRIMARY KEY (timestamp)"
            ") ENGINE = InnoDB;";

    qDebug().noquote() << "query: " + queryString;

    if (query.exec(queryString)) {
        qDebug() << "Query di creazione tabella timestamp andata a buon fine";
    }
    else{
        qDebug() << "Query di creazione tabella timestamp fallita";
        return;
    }
*/
    //inserisco la nuova entry timestamp-numero di persone rilevate a quel timestamp nella tabella timestamp
    QMap<QString, Person>::iterator pm = this->peopleMap->begin();
    QSet<QSharedPointer<Packet>> ps = pm.value().getPacketsSet();
    QString timestamp = (*ps.begin())->getTimestamp();
    QStringList tsSplit = timestamp.split(':');
    //Il timestamp ricevuto è del tipo: 22/03/2019_17:52:14
    //in questa maniera vado a memorizzare l'intera stringa tranne i secondi
    QString Timestamp = tsSplit.at(0) + tsSplit.at(1);
    qDebug().noquote() << "Timestamp delle persone nella peopleMap: " + Timestamp;

    queryString = "INSERT INTO timestamps (timestamp, count) "
                              "VALUES ('" + Timestamp + "', " + QString::number(peopleMap->count()) + ");";

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
    emit finished();
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
