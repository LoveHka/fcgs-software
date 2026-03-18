// datasource/serialreader.cpp
#include "serialreader.h"
#include "datasource/datapacket.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialReader::SerialReader(QObject *parent)
    : QObject(parent)
{
    connect(&serial, &QSerialPort::readyRead, this, &SerialReader::onReadyRead);
}

static bool isArduinoLike(const QSerialPortInfo& port)
{   
    /*
    Detecting if available port may be arduino
    */

    const auto vid = port.vendorIdentifier();
    const auto pid = port.productIdentifier();

    if (vid == 0x2341 || vid == 0x1A86 || vid == 0x10C4)
        return true;

    QString desc = port.description().toLower();
    QString manuf = port.manufacturer().toLower();

    if (desc.contains("arduino") ||
        desc.contains("ch340") ||
        desc.contains("cp210") ||
        manuf.contains("arduino"))
        return true;

    return false;
}

QList<QSerialPortInfo> getArduinoPorts()
{
    /*
    Get all ports where arduino can be inserted
    */
    QList<QSerialPortInfo> result;
    for (const auto& port : QSerialPortInfo::availablePorts()) {
        if (isArduinoLike(port))
            result.append(port);
    }
    return result;
}

void SerialReader::open(const QString &portName)
{
    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud115200);

    if (!serial.open(QIODevice::ReadOnly)) {
        qDebug() << "OPEN FAILED:" << serial.errorString();
        return;
    }

    qDebug() << "PORT OPENED";
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





