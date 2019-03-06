#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <listenerthread.h>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer();
    void setLog(QPlainTextEdit* log);

signals:
    void sig_start();

public slots:
//    void onClientConnection();
    void startToClients();

private:
    QMutex* mutex;
//    void onClientConnection(qintptr socketDescriptor);
    QPlainTextEdit *log;
    QMap<QString, QSharedPointer<Packet>> packetsMap;

protected:
    void incomingConnection(qintptr socketDescriptor);
};



#endif // MYSERVER_H
