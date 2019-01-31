#ifndef LISTENERTHREAD_H
#define LISTENERTHREAD_H

#include <QThread>
#include <iostream>
#include <QTcpSocket>
#include <QEventLoop>
#include <QPlainTextEdit>

using namespace std;

class ListenerThread : public QThread
{
    Q_OBJECT

public:
    ListenerThread();
    ListenerThread(QObject *parent, QPlainTextEdit *log, QTcpSocket *socket);
    void run() override;
    void clientSetup(QTcpSocket *socket);


public slots:
    void readFromClient();
    void sendStart();


private:
    QTcpSocket *socket;
    int socketDescriptor;
    QPlainTextEdit *log;
};

#endif // LISTENERTHREAD_H
