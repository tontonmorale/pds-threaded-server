#ifndef LISTENEROBJ_H
#define LISTENEROBJ_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"

using namespace std;

class ListenerObj : public QObject
{
    Q_OBJECT

public:
    ListenerObj();
    ListenerObj(QPlainTextEdit *log, qintptr socketDescriptor, QMutex* mutex, QMap<QString, QSharedPointer<Packet>> *packetsMap);
//    void clientSetup(QTcpSocket *socket);
    void clientSetup();    

public slots:
    void process();
    void readFromClient();
    void sendStart();

signals:
    void ready();
    void finished();


private:
    QMutex* mutex;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    QPlainTextEdit *log;
};

#endif // LISTENEROBJ_H
