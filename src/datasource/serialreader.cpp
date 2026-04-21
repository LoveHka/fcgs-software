// datasource/serialreader.cpp

#include "serialreader.h"
#include "datasource/telemetry_protocol.h"

#include <QtEndian>

#include <QDebug>
#include <QSerialPortInfo>
#include <cstring>

#include <qcontainerfwd.h>
#include <qstringview.h>
#include <qtypes.h>

SerialReader::SerialReader(QObject *parent) : QObject(parent) {
  connect(&serial, &QSerialPort::readyRead, this, &SerialReader::onReadyRead);
}

static bool isArduinoLike(const QSerialPortInfo &port) {
  /*
  Detecting if available port may be arduino
  */

  const auto vid = port.vendorIdentifier();
  const auto pid = port.productIdentifier();
  Q_UNUSED(pid);

  if (vid == 0x2341 || vid == 0x1A86 || vid == 0x10C4)
    return true;

  QString desc = port.description().toLower();
  QString manuf = port.manufacturer().toLower();

  if (desc.contains("arduino") || desc.contains("ch340") ||
      desc.contains("cp210") || manuf.contains("arduino"))
    return true;

  return false;
}

QList<QSerialPortInfo> getArduinoPorts() {
  /*
  Get all ports where arduino can be inserted
  */
  QList<QSerialPortInfo> result;
  for (const auto &port : QSerialPortInfo::availablePorts()) {
    if (isArduinoLike(port))
      result.append(port);
  }
  return result;
}

void SerialReader::open(const QString &portName) {
  serial.setPortName(portName);
  serial.setBaudRate(QSerialPort::Baud115200);
  serial.setDataBits(QSerialPort::Data8);
  serial.setParity(QSerialPort::NoParity);
  serial.setStopBits(QSerialPort::OneStop);
  serial.setFlowControl(QSerialPort::NoFlowControl);

  if (!serial.open(QIODevice::ReadWrite)) {
    qDebug() << "OPEN FAILED:" << serial.errorString();
    return;
  }

  qDebug() << "PORT OPENED";
}

void SerialReader::close() {
  if (serial.isOpen())
    serial.close();
}

void SerialReader::onReadyRead() {
  QByteArray data = serial.readAll();

  buffer.append(data);

  tryParse();
}

void SerialReader::tryParse() {
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

    if (static_cast<quint8>(buffer[1]) != TEL_SOF2) {
      buffer.remove(0, 1);
      continue;
    }

    const quint8 version = static_cast<quint8>(buffer[2]);
    const quint8 type = static_cast<quint8>(buffer[3]);

    const quint16 payloadLen = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(buffer.constData() + 4));

    const int frameSize = headerSize + payloadLen + crcSize;
    if (buffer.size() < frameSize) {
      return; // incomplete frame
    }

    const QByteArray payload = buffer.mid(headerSize, payloadLen);

    const quint16 rxCrc =
        qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(
            buffer.constData() + headerSize + payloadLen));

    const quint16 calcCrc =
        crc16_ccitt(reinterpret_cast<const uint8_t *>(payload.constData()),
                    static_cast<size_t>(payload.size()));

    if (version != TEL_VERSION || type != TEL_TYPE_DATA || rxCrc != calcCrc) {
      buffer.remove(0, 1); // resync
      continue;
    }

    if (type == TEL_TYPE_DATA && payloadLen == sizeof(DataPacket)) {
      DataPacket p{};
      std::memcpy(&p, payload.constData(), sizeof(DataPacket));
      emit packetReady(p);
    } else if (type == TEL_TYPE_MESSAGE) {
      // Преобразуем сырые байты в QString (UTF-8)
      emit messageReady(QString::fromUtf8(payload.constData(), payloadLen));
    }

    buffer.remove(0, frameSize);
  }
}

void SerialReader::sendCommand(const CommandPacket &cmd) {

  if (!serial.isOpen())
    return;

  QByteArray frame;
  frame.reserve(6 + sizeof(CommandPacket) + 2);

  frame.append(char(TEL_SOF1));
  frame.append(char(TEL_SOF2));
  frame.append(char(TEL_VERSION));
  frame.append(char(TEL_TYPE_CMD));

  const quint16 len = static_cast<quint16>(sizeof(CommandPacket));
  frame.append(char(len & 0xFF));
  frame.append(char((len >> 8) & 0xFF));
  frame.append(reinterpret_cast<const char *>(&cmd), sizeof(CommandPacket));

  const quint16 crc = crc16_ccitt(reinterpret_cast<const uint8_t *>(&cmd),
                                  sizeof(CommandPacket));
  frame.append(char(crc & 0xFF));
  frame.append(char((crc >> 8) & 0xFF));

  serial.write(frame);
}
