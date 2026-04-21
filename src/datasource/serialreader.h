// datasource/serialreader.h
#pragma once

#include "datasource/telemetry_protocol.h"
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialReader : public QObject {
  Q_OBJECT
public:
  explicit SerialReader(QObject *parent = nullptr);

  void open(const QString &portName);
  void close();
  void sendCommand(const CommandPacket &cmd);
signals:
  void packetReady(const DataPacket &packet);
  void messageReady(const QString &msg);
private slots:
  void onReadyRead();

private:
  QSerialPort serial;
  QByteArray buffer;

  void tryParse();
};

QList<QSerialPortInfo> getArduinoPorts();
