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
    QList<QPointF> *DrawOldCountMap(QString begintime, QString endtime);
    void Connects(QString slot);
    ~MyServer() override;
    static const int intervalTime = 15000;
    static const int elaborateTime = intervalTime/3;

signals:
    void start2ClientsSig(int currMinute);
    void fatalErrorSig(QString message);
    void logSig(QString message);
    void drawChartSig(QMap<QString, int> chartDataToDrawMap);
    void chartDataToDbSig(QMap<QString, Person> peopleMap);
    void getStatsSig(QString begintime, QString endtime);
    void drawMapSig(QList<QPointF> devicesCoords, QPointF maxEspCoords);
    void dbConnectedSig();
    void closeConnectionSig(QString s);

public slots:
//    void onClientConnection();
    void startToClientsSlot();
    void emitLogSlot(QString message);
    void createElaborateThreadSlot();
    void onChartDataReadySlot();
    void readyFromClientSlot(ListenerThread *lt);
    void errorFromThreadSlot(QString errorMsg);
    void emitDrawChartSlot(QMap<QString, int> chartDataToDrawMap);
    void addListenerThreadSlot(ListenerThread *lt);
    void disconnectClientSlot(QString);
    void dbConnectedSlot();
//    void drawMapSlot();
//    void timeout();

private:
    QMutex *mutex;
//    void onClientConnection(qintptr socketDescriptor);
//    QPlainTextEdit *log;
    QMap<QString, Packet> *packetsMap;
    QMap<QString, int> *packetsDetectionMap;
    QMap<QString, Esp> *espMap;
    QMap<QString, Person> *peopleMap;
    QMap<QString, int> *oldCountMap;
    int connectedClients;
    int totClients;
    int endPkClients;
    int currMinute;
//    QList<ListenerThread*> listenerThreadList;
    QMap<QString, ListenerThread*> listenerThreadPool;
    DBThread *dbthread;
    bool DBinitialized;
    QPointF maxEspCoords;
    double maxSignal;
    QPointF setMaxEspCoords(QMap<QString, Esp> *espMap);
    QList<QPointF> *devicesCoords;
    bool firstStart;
    QTimer elaborateTimer; // tempo max di attesa, da quando scade il minuto, prima di far partire
                           // l'elaborazione se non ho ricevuto tutti gli end file
    QTimer startTimer;
    void createElaborateThread();
    bool elabTimerTimeout;
    bool startCalled;
    QString tag;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};



#endif // MYSERVER_H
