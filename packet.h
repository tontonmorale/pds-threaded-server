#ifndef PACKET_H
#define PACKET_H
#include <QString>

class Packet
{
public:
   Packet();
   Packet(QString hash, QString mac, QString timestamp, int signal, QString esp, QString ssid);
   QString getHash();
   QString getMac();
   QString getEspId();
   QString getTimestamp();
   int getSignal();


private:
   QString espId;
   QString hash;
   QString mac;
   QString timestamp;
   int signal;
   QString ssid;
};

#endif // PACKET_H
