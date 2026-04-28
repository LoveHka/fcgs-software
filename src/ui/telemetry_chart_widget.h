// ui/telemetry_chart_widget.h
#pragma once

#include "datasource/telemetry_protocol.h"
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <functional>

class QChart;
class QChartView;
class QComboBox;
class QLineSeries;
class QValueAxis;

class TelemetryChartWidget : public QWidget {
  Q_OBJECT

public:
  explicit TelemetryChartWidget(const QString &defaultChannelSet,
                                QWidget *parent = nullptr);

  void appendPacket(const DataPacket &packet);
  void clear();

  void setSelectedChannelSet(const QString &channelSetName);
  QString selectedChannelSet() const;

  void setTimeWindow(qreal seconds);
  void setPointLimit(int limit);

private slots:
  void onSelectionChanged(int index);

private:
  struct ChannelSpec {
    QString seriesName;
    std::function<qreal(const DataPacket &)> getter;
  };

  struct ChannelSetSpec {
    QString displayName;
    QVector<ChannelSpec> channels;
  };

private:
  static const QVector<ChannelSetSpec> &channelSets();
  int findChannelSetIndex(const QString &name) const;

  void rebuildSeries();
  void resetViewport();
  void updateXAxis();
  void updateYAxis();
  void trimSeriesIfNeeded();

private:
  QChart *m_chart{nullptr};
  QChartView *m_chartView{nullptr};
  QComboBox *m_combo{nullptr};
  QValueAxis *m_axisX{nullptr};
  QValueAxis *m_axisY{nullptr};

  QVector<QLineSeries *> m_seriesList;
  QVector<ChannelSpec> m_activeChannels;

  qreal m_timeWindowSec{10.0};
  int m_pointLimit{1000};

  qreal m_lastX{0.0};
  bool m_hasData{false};

  QTimer *chartTimer;
};
