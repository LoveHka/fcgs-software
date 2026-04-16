// ui/main_window.h

#pragma once

#include <QMainWindow>
#include <array>

class SerialReader;
class TelemetryChartWidget;
class Telemetry3DWidget;
class TrajectoryWidget;
class TelemetryValuesWidget;
struct DataPacket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onPacket(const DataPacket& p);

private:
    SerialReader* reader{nullptr};
    std::array<TelemetryChartWidget*, 3> m_charts{};
    Telemetry3DWidget* m_3dWidget{nullptr};
    TrajectoryWidget* m_trajectoryWidget{nullptr};
    TelemetryValuesWidget* m_valuesWidget{nullptr};
};
