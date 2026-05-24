#include "telemetry_logger.h"

TelemetryLogger::TelemetryLogger(QObject *parent) : QObject(parent) {}

TelemetryLogger::~TelemetryLogger() { stopLogging(); }

bool TelemetryLogger::startLogging(const QString &fileName) {
  if (m_file.isOpen()) {
    stopLogging();
  }

  m_file.setFileName(fileName);
  // Открываем файл для записи (Truncate очищает старый файл, Append дописывает)
  if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text |
                   QIODevice::Truncate)) {
    qDebug() << "Не удалось открыть файл для логирования:" << fileName;
    return false;
  }

  m_stream.setDevice(&m_file);

  // Пишем заголовок столбцов (CSV Header)
  // Используем ; как разделитель для Excel
  m_stream << "Time;ax;ay;az;vx;vy;vz;sx;sy;sz;mx;my;mz;H;gx;gy;gz;angx;angy;"
              "angz;P;T;BarAlt;Voltage;d;Tcycle\n";
  m_stream.flush(); // Гарантируем запись заголовка

  return true;
}

void TelemetryLogger::stopLogging() {
  if (m_file.isOpen()) {
    m_stream.flush();
    m_file.close();
  }
}

void TelemetryLogger::onPacketReceived(const DataPacket &packet) {
  if (!m_file.isOpen()) {
    return;
  }

  m_stream << packetToCsv(packet);

  // Опционально: можно делать flush реже, например, каждые 10 пакетов,
  // чтобы не тормозить интерфейс, но для телеметрии лучше писать сразу,
  // чтобы не потерять данные при краше.
  m_stream.flush();
}

QString TelemetryLogger::packetToCsv(const DataPacket &packet) const {
  // Helper lambda to format float with comma separator for Excel compatibility
  // in RU locale
  auto fmt = [](float val) -> QString {
    QString s = QString::number(val, 'f', 4); // 4 знака после запятой
    return s.replace('.', ','); // Excel любит запятые в русских настройках
  };

  QStringList fields;
  fields << QString::number(packet.time, 'f', 0); // Время в мс, целое
  fields << fmt(packet.ax);
  fields << fmt(packet.ay);
  fields << fmt(packet.az);
  fields << fmt(packet.vx);
  fields << fmt(packet.vy);
  fields << fmt(packet.vz);
  fields << fmt(packet.sx);
  fields << fmt(packet.sy);
  fields << fmt(packet.sz);
  fields << fmt(packet.mx);
  fields << fmt(packet.my);
  fields << fmt(packet.mz);
  fields << fmt(packet.H);
  fields << fmt(packet.gx);
  fields << fmt(packet.gy);
  fields << fmt(packet.gz);
  fields << fmt(packet.angx);
  fields << fmt(packet.angy);
  fields << fmt(packet.angz);
  fields << fmt(packet.P);
  fields << fmt(packet.T);
  fields << fmt(packet.BarAlt);
  fields << fmt(packet.Voltage);
  fields << fmt(packet.d);
  fields << fmt(packet.Tcycle);

  return fields.join(';') + "\n";
}
