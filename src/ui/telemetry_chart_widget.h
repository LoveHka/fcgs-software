// ui/telemetry_chart_widget.h

#pragma once

#include <QWidget>

class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;

class TelemetryChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TelemetryChartWidget(
        const QString& title,
        qreal yMin = -200.0,
        qreal yMax = 200.0,
        qreal timeWindowSec = 10.0,
        QWidget* parent = nullptr
    );

    void appendPoint(qreal x, qreal y);
    void clear();

    void setYAxisRange(qreal yMin, qreal yMax);
    void setTimeWindow(qreal seconds);
    void setPointLimit(int limit);

private:
    void updateXAxis();
    void trimSeriesIfNeeded();

private:
    QChart* m_chart{nullptr};
    QChartView* m_chartView{nullptr};
    QLineSeries* m_series{nullptr};
    QValueAxis* m_axisX{nullptr};
    QValueAxis* m_axisY{nullptr};

    qreal m_timeWindowSec{10.0};
    int m_pointLimit{1000};

    qreal m_lastX{0.0};
    bool m_hasData{false};
};