#ifndef ESP_H
#define ESP_H

#include <QPointF>
#include <QString>
#include <QTcpSocket>

class Esp
{
public:
    Esp();
    Esp(QString id, QString mac, QPointF coord);
//    QString getMac();
    QString getId();
//    QTcpSocket* getSocket();
//    void setSocket(QTcpSocket *socket);
//    QPointF getPoint();

private:
    QString id;
    QString mac;
    QPointF coord;
};

#endif // ESP_H
