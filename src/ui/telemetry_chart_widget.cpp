// ui/telemetry_chart_widget.cpp
#include "ui/telemetry_chart_widget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
const QVector<QColor> &seriesPalette() {
  static const QVector<QColor> palette = {
      QColor(0x1f, 0x77, 0xb4), QColor(0xd6, 0x27, 0x28),
      QColor(0x2c, 0xa0, 0x2c), QColor(0xff, 0x7f, 0x0e),
      QColor(0x94, 0x67, 0xbd), QColor(0x17, 0xbe, 0xcf),
      QColor(0x8c, 0x56, 0x4b), QColor(0xe3, 0x77, 0xc2)};
  return palette;
}
} // namespace

TelemetryChartWidget::TelemetryChartWidget(const QString &defaultChannelSet,
                                           QWidget *parent)
    : QWidget(parent), m_chart(new QChart()),
      m_chartView(new QChartView(m_chart, this)), m_combo(new QComboBox(this)),
      m_axisX(new QValueAxis(m_chart)), m_axisY(new QValueAxis(m_chart)) {
  auto *outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(4, 4, 4, 4);
  outerLayout->setSpacing(4);

  auto *controlsLayout = new QHBoxLayout();
  controlsLayout->setContentsMargins(0, 0, 0, 0);
  controlsLayout->setSpacing(6);

  auto *label = new QLabel(QStringLiteral("Канал:"), this);
  controlsLayout->addWidget(label);
  controlsLayout->addWidget(m_combo, 1);

  outerLayout->addLayout(controlsLayout);
  outerLayout->addWidget(m_chartView, 1);

  m_chart->legend()->setVisible(true);
  m_chart->legend()->setAlignment(Qt::AlignLeft);
  m_chart->legend()->setContentsMargins(0, 0, 0, 0);
  m_chart->legend()->setVisible(false);
  // m_chart->setMargins(QMargins(200, 20, 20, 20));
  m_chart->setBackgroundRoundness(0.0);

  m_chart->setAnimationOptions(QChart::NoAnimation);
  m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  m_axisX->setTitleText(QStringLiteral("Time, s"));
  m_axisX->setTitleVisible(false);
  m_axisX->setLabelFormat("%.2f");

  // m_axisY->setTitleText(QStringLiteral("Value"));
  m_axisY->setTitleVisible(false);
  m_axisY->setLabelFormat("%g");

  QFont axFont = m_axisY->labelsFont();
  axFont.setPointSize(6);
  m_axisY->setLabelsFont(axFont);

  m_chart->addAxis(m_axisX, Qt::AlignBottom);
  m_chart->addAxis(m_axisY, Qt::AlignLeft);

  const auto &sets = channelSets();
  for (const auto &set : sets) {
    m_combo->addItem(set.displayName);
  }

  connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &TelemetryChartWidget::onSelectionChanged);

  const int defaultIndex = findChannelSetIndex(defaultChannelSet);
  m_combo->setCurrentIndex(defaultIndex >= 0 ? defaultIndex : 0);

  rebuildSeries();

  // Настройка и запуск таймера на 30 FPS (~33 миллисекунды)
  m_renderTimer = new QTimer(this);
  connect(m_renderTimer, &QTimer::timeout, this,
          &TelemetryChartWidget::onRenderTimer);
  m_renderTimer->start(33);
}

const QVector<TelemetryChartWidget::ChannelSetSpec> &
TelemetryChartWidget::channelSets() {
  static const QVector<ChannelSetSpec> sets = {
      {QStringLiteral("Ускорения (ax, ay, az)"),
       {
           {QStringLiteral("ax"),
            [](const DataPacket &p) { return static_cast<qreal>(p.ax); }},
           {QStringLiteral("ay"),
            [](const DataPacket &p) { return static_cast<qreal>(p.ay); }},
           {QStringLiteral("az"),
            [](const DataPacket &p) { return static_cast<qreal>(p.az); }},
       }},
      {QStringLiteral("Скорости (vx, vy, vz)"),
       {
           {QStringLiteral("vx"),
            [](const DataPacket &p) { return static_cast<qreal>(p.vx); }},
           {QStringLiteral("vy"),
            [](const DataPacket &p) { return static_cast<qreal>(p.vy); }},
           {QStringLiteral("vz"),
            [](const DataPacket &p) { return static_cast<qreal>(p.vz); }},
       }},
      {QStringLiteral("Координаты (sx, sy, sz)"),
       {
           {QStringLiteral("sx"),
            [](const DataPacket &p) { return static_cast<qreal>(p.sx); }},
           {QStringLiteral("sy"),
            [](const DataPacket &p) { return static_cast<qreal>(p.sy); }},
           {QStringLiteral("sz"),
            [](const DataPacket &p) { return static_cast<qreal>(p.sz); }},
       }},
      {QStringLiteral("Магнитометр (mx, my, mz)"),
       {
           {QStringLiteral("mx"),
            [](const DataPacket &p) { return static_cast<qreal>(p.mx); }},
           {QStringLiteral("my"),
            [](const DataPacket &p) { return static_cast<qreal>(p.my); }},
           {QStringLiteral("mz"),
            [](const DataPacket &p) { return static_cast<qreal>(p.mz); }},
       }},
      {QStringLiteral("Гироскоп (gx, gy, gz)"),
       {
           {QStringLiteral("gx"),
            [](const DataPacket &p) { return static_cast<qreal>(p.gx); }},
           {QStringLiteral("gy"),
            [](const DataPacket &p) { return static_cast<qreal>(p.gy); }},
           {QStringLiteral("gz"),
            [](const DataPacket &p) { return static_cast<qreal>(p.gz); }},
       }},
      {QStringLiteral("Углы (angx, angy, angz)"),
       {
           {QStringLiteral("angx"),
            [](const DataPacket &p) { return static_cast<qreal>(p.angx); }},
           {QStringLiteral("angy"),
            [](const DataPacket &p) { return static_cast<qreal>(p.angy); }},
           {QStringLiteral("angz"),
            [](const DataPacket &p) { return static_cast<qreal>(p.angz); }},
       }},
      {QStringLiteral("Напряжение"),
       {
           {QStringLiteral("В"),
            [](const DataPacket &p) { return static_cast<qreal>(p.Voltage); }},
       }},
      {QStringLiteral("Направление H"),
       {
           {QStringLiteral("H"),
            [](const DataPacket &p) { return static_cast<qreal>(p.H); }},
       }},
      {QStringLiteral("Давление P"),
       {
           {QStringLiteral("P"),
            [](const DataPacket &p) { return static_cast<qreal>(p.P); }},
       }},
      {QStringLiteral("Температура T"),
       {
           {QStringLiteral("T"),
            [](const DataPacket &p) { return static_cast<qreal>(p.T); }},
       }},
      {QStringLiteral("Баро-высота BarAlt"),
       {
           {QStringLiteral("BarAlt"),
            [](const DataPacket &p) { return static_cast<qreal>(p.BarAlt); }},
       }},
      {QStringLiteral("Дистанция d"),
       {
           {QStringLiteral("d"),
            [](const DataPacket &p) { return static_cast<qreal>(p.d); }},
       }},
      {QStringLiteral("Tcycle"),
       {
           {QStringLiteral("Tcycle"),
            [](const DataPacket &p) { return static_cast<qreal>(p.Tcycle); }},
       }}};

  return sets;
}

int TelemetryChartWidget::findChannelSetIndex(const QString &name) const {
  const auto &sets = channelSets();
  for (int i = 0; i < sets.size(); ++i) {
    if (sets[i].displayName == name) {
      return i;
    }
  }
  return -1;
}

void TelemetryChartWidget::rebuildSeries() {
  for (auto *series : m_seriesList) {
    m_chart->removeSeries(series);
    series->deleteLater();
  }
  m_seriesList.clear();
  m_activeChannels.clear();
  m_pendingData.clear();

  const int selectedIndex = m_combo ? m_combo->currentIndex() : -1;
  const auto &sets = channelSets();

  if (selectedIndex < 0 || selectedIndex >= sets.size()) {
    resetViewport();
    return;
  }

  const auto &selectedSet = sets[selectedIndex];
  m_activeChannels = selectedSet.channels;

  const auto &palette = seriesPalette();

  for (int i = 0; i < m_activeChannels.size(); ++i) {
    auto *series = new QLineSeries();
    series->setName(m_activeChannels[i].seriesName);

    QPen pen(palette[i % palette.size()]);
    pen.setWidthF(2.0);
    series->setPen(pen);

    // Включаем встроенное OpenGL ускорение QtCharts для отрисовки линий
    // series->setUseOpenGL(true);

    m_chart->addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);

    m_seriesList.append(series);
  }

  // Резервируем буферы под новые серии
  m_pendingData.resize(m_seriesList.size());
  m_yBoundsInitialized = false;

  resetViewport();
}

void TelemetryChartWidget::resetViewport() {
  m_lastX = 0.0;
  m_hasData = false;

  m_axisX->setRange(0.0, m_timeWindowSec);
  m_axisY->setRange(-1.0, 1.0);
}

void TelemetryChartWidget::appendPacket(const DataPacket &packet) {
  if (m_activeChannels.isEmpty() || m_seriesList.isEmpty() ||
      m_pendingData.isEmpty()) {
    return;
  }

  const qreal x = static_cast<qreal>(packet.time) / 1000.0;

  // Быстро записываем данные во временные буферы, не затрагивая UI сигналы
  for (int i = 0; i < m_seriesList.size() && i < m_activeChannels.size(); ++i) {
    const qreal y = m_activeChannels[i].getter(packet);
    m_pendingData[i].append(QPointF(x, y));

    // Вычисляем границы Y на лету во время добавления пакета
    if (!m_yBoundsInitialized) {
      m_currentMinY = m_currentMaxY = y;
      m_yBoundsInitialized = true;
    } else {
      if (y < m_currentMinY)
        m_currentMinY = y;
      if (y > m_currentMaxY)
        m_currentMaxY = y;
    }
  }

  m_lastX = x;
  m_hasData = true;
}

void TelemetryChartWidget::onRenderTimer() {
  // Если новых данных нет или буферы пусты — пропускаем кадр
  if (!m_hasData || m_pendingData.isEmpty() ||
      m_pendingData.first().isEmpty()) {
    return;
  }

  // Отключаем сигналы чарта, чтобы избежать промежуточных перерисовок
  m_chart->blockSignals(true);

  for (int i = 0; i < m_seriesList.size(); ++i) {
    auto *series = m_seriesList[i];

    // Быстро извлекаем текущие точки через вектор без полного копирования
    // структуры данных
    QVector<QPointF> points = series->pointsVector();

    // Сливаем старые точки с накопленным за 33мс буфером пакетов
    points.append(m_pendingData[i]);
    m_pendingData[i].clear();

    // Обрезаем данные по лимиту количества точек прямо внутри вектора
    if (points.size() > m_pointLimit) {
      points.remove(0, points.size() - m_pointLimit);
    }

    // Передаем весь обновленный массив обратно в серию за одну операцию
    series->replace(points);
  }

  // Включаем сигналы обратно для финального обновления кадра
  m_chart->blockSignals(false);

  updateXAxis();
  updateYAxis();
}

void TelemetryChartWidget::clear() {
  for (auto *series : m_seriesList) {
    series->clear();
  }
  for (auto &buffer : m_pendingData) {
    buffer.clear();
  }
  m_yBoundsInitialized = false;
  resetViewport();
}

void TelemetryChartWidget::setSelectedChannelSet(
    const QString &channelSetName) {
  const int index = findChannelSetIndex(channelSetName);
  if (index < 0 || !m_combo) {
    return;
  }

  if (m_combo->currentIndex() == index) {
    rebuildSeries();
    return;
  }

  m_combo->setCurrentIndex(index);
}

QString TelemetryChartWidget::selectedChannelSet() const {
  return m_combo ? m_combo->currentText() : QString();
}

void TelemetryChartWidget::setTimeWindow(qreal seconds) {
  m_timeWindowSec = std::max<qreal>(0.1, seconds);
  updateXAxis();
}

void TelemetryChartWidget::setPointLimit(int limit) {
  m_pointLimit = std::max(2, limit);
  updateYAxis();
}

void TelemetryChartWidget::onSelectionChanged(int) { rebuildSeries(); }

void TelemetryChartWidget::updateXAxis() {
  if (!m_hasData) {
    m_axisX->setRange(0.0, m_timeWindowSec);
    return;
  }

  const qreal maxX = std::max<qreal>(m_lastX, 0.001);
  const qreal minX = std::max<qreal>(0.0, maxX - m_timeWindowSec);

  if (maxX <= minX)
    return;

  // Округляем до 2 знаков после запятой для стабильности сетки
  const qreal newMin = std::round(minX * 100.0) / 100.0;
  const qreal newMax = std::round(maxX * 100.0) / 100.0;

  // Обновляем диапазон ТОЛЬКО если он изменился существенно
  // Это предотвращает постоянный пересчет макета и исчезновение цифр
  if (std::abs(m_axisX->min() - newMin) > 0.01 ||
      std::abs(m_axisX->max() - newMax) > 0.01) {
    m_axisX->setRange(newMin, newMax);
  }
}

void TelemetryChartWidget::updateYAxis() {
  if (!m_yBoundsInitialized || m_seriesList.isEmpty()) {
    m_axisY->setRange(-1.0, 1.0);
    return;
  }

  const qreal span = m_currentMaxY - m_currentMinY;
  qreal padding = 0.0;

  if (span <= 0.000001) {
    padding = std::max<qreal>(0.5, std::abs(m_currentMaxY) * 0.1);
  } else {
    padding = std::max<qreal>(span * 0.10, 0.5);
  }

  // Округляем границы до 2 знаков после запятой
  const qreal newMin = std::round((m_currentMinY - padding) * 100.0) / 100.0;
  const qreal newMax = std::round((m_currentMaxY + padding) * 100.0) / 100.0;

  // Обновляем ТОЛЬКО при существенном изменении
  if (std::abs(m_axisY->min() - newMin) > 0.01 ||
      std::abs(m_axisY->max() - newMax) > 0.01) {
    m_axisY->setRange(newMin, newMax);
  }
}
