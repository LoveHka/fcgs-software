#include "serialreader.h"
#include "datasource/datapacket.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialReader::SerialReader(QObject *parent)
    : QObject(parent)
{
    connect(&serial, &QSerialPort::readyRead, this, &SerialReader::onReadyRead);
}

void SerialReader::open(const QString &portName)
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        qDebug() << "Port: " << port.portName();
    }
    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud115200);
    if (serial.open(QIODevice::ReadOnly)) {
            qDebug() << "PORT OPENED";
            }
}

void SerialReader::onReadyRead()
{   
    qDebug() << "onReadyRead )))";   
    QByteArray data = serial.readAll();
    qDebug() << data;
    buffer.append(data);
    
    tryParse();
    
}

void SerialReader::tryParse()
{
    while (buffer.contains('\n')) {
        int idx = buffer.indexOf('\n');
        QByteArray line = buffer.left(idx);
        buffer.remove(0, idx + 1);
        
        line = line.trimmed();

        QList<QByteArray> parts = line.split(',');

        if (parts.size() < 2) continue;

        DataPacket p;
        p.time = parts[0].toFloat();
        p.ax = parts[1].toFloat();
        
        qDebug() << "EMIT PACKET";
        emit packetReady(p);
    }
}





