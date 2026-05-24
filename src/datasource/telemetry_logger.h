// datasource/telemetry_logger.h
#pragma once

#include "datasource/telemetry_protocol.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QTextStream>

class TelemetryLogger : public QObject {
  Q_OBJECT
public:
  explicit TelemetryLogger(QObject *parent = nullptr);
  ~TelemetryLogger();

  // Начать запись в указанный файл
  bool startLogging(const QString &fileName);

  // Остановить запись
  void stopLogging();

  // Проверка, идет ли запись
  bool isLogging() const { return m_file.isOpen(); }

public slots:
  // Этот слот будем подключать к сигналу packetReady
  void onPacketReceived(const DataPacket &packet);

private:
  QFile m_file;
  QTextStream m_stream;

  // Формирует строку CSV из пакета данных
  QString packetToCsv(const DataPacket &packet) const;
};
