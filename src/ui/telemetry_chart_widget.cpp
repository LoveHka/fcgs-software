#include "ui/telemetry_chart_widget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QHBoxLayout>
#include <QPainter>
#include <QtGlobal>

TelemetryChartWidget::TelemetryChartWidget(
    const QString& title,
    qreal yMin,
    qreal yMax,
    qreal timeWindowSec,
    QWidget* parent)
    : QWidget(parent)
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart, this))
    , m_series(new QLineSeries(m_chart))
    , m_axisX(new QValueAxis(m_chart))
    , m_axisY(new QValueAxis(m_chart))
    , m_timeWindowSec(timeWindowSec)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartView);

    m_chart->addSeries(m_series);
    m_chart->setTitle(title);
    m_chart->legend()->hide();
    m_chart->setMargins(QMargins(0, 0, 0, 0));
    m_chart->setBackgroundRoundness(0.0);

    m_axisX->setTitleText("Time, s");
    m_axisX->setLabelFormat("%.2f");
    m_axisX->setRange(0.0, m_timeWindowSec);

    m_axisY->setTitleText("Value");
    m_axisY->setLabelFormat("%.2f");
    m_axisY->setRange(yMin, yMax);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QPen pen = m_series->pen();
    pen.setWidthF(2.0);
    m_series->setPen(pen);
}

void TelemetryChartWidget::appendPoint(qreal x, qreal y)
{
    m_series->append(x, y);
    m_lastX = x;
    m_hasData = true;

    trimSeriesIfNeeded();
    updateXAxis();
}

void TelemetryChartWidget::clear()
{
    m_series->clear();
    m_lastX = 0.0;
    m_hasData = false;
    m_axisX->setRange(0.0, m_timeWindowSec);
}

void TelemetryChartWidget::setYAxisRange(qreal yMin, qreal yMax)
{
    m_axisY->setRange(yMin, yMax);
}

void TelemetryChartWidget::setTimeWindow(qreal seconds)
{
    m_timeWindowSec = qMax<qreal>(0.1, seconds);
    updateXAxis();
}

void TelemetryChartWidget::setPointLimit(int limit)
{
    m_pointLimit = qMax(2, limit);
    trimSeriesIfNeeded();
}

void TelemetryChartWidget::updateXAxis()
{
    if (!m_hasData) {
        m_axisX->setRange(0.0, m_timeWindowSec);
        return;
    }

    const qreal maxX = qMax(m_lastX, 0.001);
    const qreal minX = qMax<qreal>(0.0, maxX - m_timeWindowSec);

    if (maxX <= minX) {
        m_axisX->setRange(minX, minX + 0.001);
        return;
    }

    m_axisX->setRange(minX, maxX);
}

void TelemetryChartWidget::trimSeriesIfNeeded()
{
    const int count = m_series->count();
    if (count <= m_pointLimit) {
        return;
    }

    const int removeCount = count - m_pointLimit;
    m_series->removePoints(0, removeCount);
}