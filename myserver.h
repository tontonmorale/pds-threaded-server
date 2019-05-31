#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "listenerthread.h"
#include "packet.h"
#include "person.h"
#include "esp.h"
#include "dbthread.h"
#include "elaboratethread.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer(QObject *parent = nullptr);
    void confFromFile();
    void init();
    void SendToDB();
    QList<QPointF> *DrawOldCountMap(QString begintime, QString endtime);
    void Connects(QString slot);
    ~MyServer() override;

signals:
    void start2ClientsSig();
    void fatalErrorSig(QString message);
    void logSig(QString message);
    void DBsig(QMap<QString, Person> *peopleMap);
    void drawRuntimeChartSig(QList<QPointF> *peopleCounter);

public slots:
//    void onClientConnection();
    void startToClientsSlot();
    void emitLogSlot(QString message);
    void createElaborateThreadSlot();
    void dataForDbSlot();
    void readyFromClientSlot();
    void clearPeopleMapSlot();
    void errorFromThreadSlot(QString errorMsg);
    void emitDrawRuntimeChartSignalSlot();

private:
    QMutex* mutex;
//    void onClientConnection(qintptr socketDescriptor);
//    QPlainTextEdit *log;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QMap<QString, Esp> *espMap;
    QMap<QString, Person> *peopleMap;
    QMap<QString, int> *oldCountMap;
    int connectedClients;
    int totClients;
    int endPkClients;
    int currMinute;
    QList<ListenerThread*> *listenerThreadList;
    DBThread *dbthread;
    bool DBinitialized;
    QPointF maxEspCoords;
    QPointF setMaxEspCoords(QMap<QString, Esp> *espMap);
    QList<QPointF> *devicesCoords;
    QList<QPointF> *peopleCounter;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};



#endif // MYSERVER_H
