// ui/main_window.cpp

#include "main_window.h"

#include "calibration/calibrationcontroller.h"
#include "datasource/serialreader.h"       // Прием телеметрии
#include "datasource/telemetry_protocol.h" // Структура телеметрии
#include "ui/magcalibrationwidget.h"
#include "ui/telemetry_3d_widget.h"    // Виджет для 3Д самолетика
#include "ui/telemetry_chart_widget.h" // Виджет для графиков
#include "ui/telemetry_messages_widget.h"
#include "ui/telemetry_values_widget.h" // Виджет для всей телеметрии, отображается всегда
#include "ui/trajectory_widget.h" // Виджет для траектории с видом сверху

#include <QComboBox>
#include <QElapsedTimer>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <qdatetime.h>
#include <qdebug.h>

namespace {
QFrame *createPanel(QWidget *parent = nullptr) {
  auto *frame = new QFrame(parent);
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFrameShadow(QFrame::Plain);
  frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  return frame;
}

QGroupBox *createGroup(const QString &title, QWidget *parent = nullptr) {
  auto *group = new QGroupBox(title, parent);
  group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  return group;
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("FCGS Telemetry");
  resize(1400, 900);

  auto *central = new QWidget(this);
  auto *rootLayout = new QHBoxLayout(central);
  rootLayout->setContentsMargins(4, 4, 4, 4);
  rootLayout->setSpacing(4);

  // Left side: Tab widget for charts + 3D + trajectory
  auto *leftTabs = new QTabWidget(central);
  leftTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // Tab 1: Charts
  auto *chartsTab = new QWidget(leftTabs);
  auto *chartsLayout = new QVBoxLayout(chartsTab);
  chartsLayout->setContentsMargins(4, 4, 4, 4);
  chartsLayout->setSpacing(4);

  m_charts[0] = new TelemetryChartWidget(
      QStringLiteral("Ускорения (ax, ay, az)"), chartsTab);
  m_charts[1] = new TelemetryChartWidget(
      QStringLiteral("Гироскоп (gx, gy, gz)"), chartsTab);
  m_charts[2] =
      new TelemetryChartWidget(QStringLiteral("Температура T"), chartsTab);

  for (auto *chart : m_charts) {
    chartsLayout->addWidget(chart, 1);
  }

  leftTabs->addTab(chartsTab, QStringLiteral("Графики"));

  // Tab 2: 3D visualization
  m_3dWidget = new Telemetry3DWidget(leftTabs);
  leftTabs->addTab(m_3dWidget, QStringLiteral("3D"));

  // Tab 3: Trajectory
  m_trajectoryWidget = new TrajectoryWidget(leftTabs);
  leftTabs->addTab(m_trajectoryWidget, QStringLiteral("Траектория"));

  rootLayout->addWidget(leftTabs, 2);

  // Right side: Telemetry values panel
  auto *rightPanel = createPanel(central);
  rightPanel->setMaximumWidth(350);
  auto *rightLayout = new QVBoxLayout(rightPanel);
  rightLayout->setContentsMargins(4, 4, 4, 4);
  rightLayout->setSpacing(4);

  m_valuesWidget = new TelemetryValuesWidget(rightPanel);
  rightLayout->addWidget(m_valuesWidget, 1);

  rootLayout->addWidget(rightPanel, 1);

  //  Создаём виджет сообщений - логов
  m_msgWidget = new TelemetryMessagesWidget(leftTabs);
  leftTabs->addTab(m_msgWidget, QStringLiteral("Лог"));

  setCentralWidget(central);

  // Initialize serial reader
  reader = new SerialReader(this);
  connect(reader, &SerialReader::packetReady, this, &MainWindow::onPacket);

  connect(reader, &SerialReader::messageReady, m_msgWidget,
          &TelemetryMessagesWidget::appendMessage);

  m_calibrationController = new CalibrationController(reader, this);

  m_magCalWidget = new MagCalibrationWidget(m_calibrationController, leftTabs);
  leftTabs->addTab(m_magCalWidget, QStringLiteral("Калибровка MAG"));

  const auto ports = getArduinoPorts();

  if (ports.isEmpty()) {
    QMessageBox::critical(this, "Error", "No Arduino-like ports found");
    return;
  }

  QStringList portNames;
  portNames.reserve(ports.size());

  for (const auto &p : ports) {
    portNames << p.portName() + " (" + p.description() + ")";
  }

  bool ok = false;
  const QString selected =
      QInputDialog::getItem(this, "Select Arduino Port",
                            "Available ports:", portNames, 0, false, &ok);

  if (!ok) {
    return;
  }

  const int index = portNames.indexOf(selected);
  if (index >= 0) {
    reader->open(ports[index].portName());
  }
}

void MainWindow::onPacket(const DataPacket &p) {
  static QElapsedTimer timer;
  if (!timer.isValid())
    timer.start();
  timer.restart();
  m_charts[0]->appendPacket(p);
  // m_charts[1]->appendPacket(p);
  // m_charts[2]->appendPacket(p);
  qDebug() << "Time spended for charts: " << timer.elapsed();
  timer.restart();
  m_3dWidget->updateOrientation(p);
  qDebug() << "Time spended for 3D: " << timer.elapsed();
  timer.restart();
  m_trajectoryWidget->updatePosition(p);
  qDebug() << "Time spended for trajectory_widget: " << timer.elapsed();
  timer.restart();
  m_valuesWidget->updateValues(p);
  qDebug() << "Time spended for values: " << timer.elapsed();
  timer.restart();
}
