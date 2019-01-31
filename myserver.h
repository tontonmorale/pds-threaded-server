#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <listenerthread.h>
#include <QEventLoop>
#include <QPlainTextEdit>


class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    MyServer();
    void setLog(QPlainTextEdit* log);

signals:
    void sig_start();

public slots:
    void onClientConnection();

private:
    void onClientConnection(qintptr socketDescriptor);
    QPlainTextEdit *log;
};

#endif // MYSERVER_H
