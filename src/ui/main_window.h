// ui/main_window.h

#pragma once

#include "datasource/telemetry_logger.h"
#include <QMainWindow>
#include <array>

class SerialReader;
class TelemetryChartWidget;
class Telemetry3DWidget;
class TrajectoryWidget;
class TelemetryValuesWidget;
class CalibrationController;
class MagCalibrationWidget;
class TelemetryMessagesWidget;
class TelemetryLogger;
struct DataPacket;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private slots:
  void onPacket(const DataPacket &p);

private:
  SerialReader *reader{nullptr};
  CalibrationController *m_calibrationController{nullptr};
  std::array<TelemetryChartWidget *, 3> m_charts{};
  Telemetry3DWidget *m_3dWidget{nullptr};
  TrajectoryWidget *m_trajectoryWidget{nullptr};
  TelemetryValuesWidget *m_valuesWidget{nullptr};
  MagCalibrationWidget *m_magCalWidget{nullptr};
  TelemetryMessagesWidget *m_msgWidget{nullptr};

  TelemetryLogger *m_logger;
};
