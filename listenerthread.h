#ifndef LISTENERTHREAD_H
#define LISTENERTHREAD_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QMutex>
#include "packet.h"

using namespace std;

class ListenerThread : public QThread
{
    Q_OBJECT

public:
    ListenerThread();
    ListenerThread(QObject *parent, QPlainTextEdit *log, qintptr socketDescriptor, QMutex* mutex, QMap<QString, QSharedPointer<Packet>> *packetsMap);
    void run() override;
    void clientSetup();


public slots:
    void readFromClient();
    void sendStart();


private:
    QMutex* mutex;
    QMap<QString, QSharedPointer<Packet>> *packetsMap;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    QPlainTextEdit *log;
};

#endif // LISTENERTHREAD_H
