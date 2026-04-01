// datasource/serialreader.cpp
#include "serialreader.h"
#include <QtEndian>
#include "datasource/telemetry_protocol.h"
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
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadOnly)) {
        qDebug() << "OPEN FAILED:" << serial.errorString();
        return;
    }

    qDebug() << "PORT OPENED";
}

void SerialReader::onReadyRead()
{   
       
    QByteArray data = serial.readAll();
    
    buffer.append(data);
    
    tryParse();
}

void SerialReader::tryParse()
{
    constexpr int headerSize = 6; // SOF1 SOF2 version type lenLo lenHi
    constexpr int crcSize = 2;

    while (true) {
        // Find sync byte 0xAA
        int start = buffer.indexOf(char(TEL_SOF1));
        if (start < 0) {
            buffer.clear();
            return;
        }

        // Drop garbage before sync
        if (start > 0) {
            buffer.remove(0, start);
        }

        if (buffer.size() < headerSize) {
            return; // need more bytes
        }

        if ((quint8)buffer[1] != TEL_SOF2) {
            buffer.remove(0, 1);
            continue;
        }

        quint8 version = (quint8)buffer[2];
        quint8 type    = (quint8)buffer[3];

        quint16 payloadLen = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar*>(buffer.constData() + 4)
        );

        const int frameSize = headerSize + payloadLen + crcSize;
        if (buffer.size() < frameSize) {
            return; // incomplete frame
        }

        QByteArray payload = buffer.mid(headerSize, payloadLen);

        quint16 rxCrc = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar*>(buffer.constData() + headerSize + payloadLen)
        );

        quint16 calcCrc = crc16_ccitt(
            reinterpret_cast<const uint8_t*>(payload.constData()),
            payload.size()
        );

        if (version != TEL_VERSION || type != TEL_TYPE_DATA || rxCrc != calcCrc) {
            buffer.remove(0, 1); // resync
            continue;
        }

        if (payloadLen == sizeof(DataPacket)) {
            DataPacket p{};
            std::memcpy(&p, payload.constData(), sizeof(DataPacket));
            emit packetReady(p);
        }

        buffer.remove(0, frameSize);
    }
}





