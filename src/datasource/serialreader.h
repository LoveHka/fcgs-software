#pragma once

#include <QObject>
#include <QSerialPort>
#include "datapacket.h"

class SerialReader : public QObject
{
    Q_OBJECT
public:
    explicit SerialReader(QObject* parent = nullptr);
    
    void open(const QString& portName);

signals:
    void packetReady(const DataPacket& packet);

private slots:
    void onReadyRead();

private:
    QSerialPort serial;
    QByteArray buffer;

    void tryParse();
};
