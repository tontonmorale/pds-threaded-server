#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <listenerobj.h>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"
#include "esp.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer(QObject *parent = 0);
    void setLog(QPlainTextEdit* log);
    void confFromFile();
    void init();

signals:
    void sig_start();
    void error(QString message);

public slots:
//    void onClientConnection();
    void startToClients();

private:
    QMutex* mutex;
//    void onClientConnection(qintptr socketDescriptor);
    QPlainTextEdit *log;
    QMap<QString, QSharedPointer<Packet>> packetsMap;
    shared_ptr<QMap<QString, Esp>> espMap;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};



#endif // MYSERVER_H
