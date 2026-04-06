// ui/telemetry_chart_widget.cpp

#include "ui/telemetry_chart_widget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>



namespace
{
const QVector<QColor>& seriesPalette()
{
    static const QVector<QColor> palette = {
        QColor(0x1f, 0x77, 0xb4),
        QColor(0xd6, 0x27, 0x28),
        QColor(0x2c, 0xa0, 0x2c),
        QColor(0xff, 0x7f, 0x0e),
        QColor(0x94, 0x67, 0xbd),
        QColor(0x17, 0xbe, 0xcf),
        QColor(0x8c, 0x56, 0x4b),
        QColor(0xe3, 0x77, 0xc2)
    };
    return palette;
}
} // namespace

TelemetryChartWidget::TelemetryChartWidget(
    const QString& defaultChannelSet,
    QWidget* parent)
    : QWidget(parent)
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart, this))
    , m_combo(new QComboBox(this))
    , m_axisX(new QValueAxis(m_chart))
    , m_axisY(new QValueAxis(m_chart))
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(4, 4, 4, 4);
    outerLayout->setSpacing(4);

    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(6);

    auto* label = new QLabel(QStringLiteral("Канал:"), this);
    controlsLayout->addWidget(label);
    controlsLayout->addWidget(m_combo, 1);

    outerLayout->addLayout(controlsLayout);
    outerLayout->addWidget(m_chartView, 1);

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignLeft);
    m_chart->setMargins(QMargins(0, 0, 0, 0));
    m_chart->setBackgroundRoundness(0.0);

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_axisX->setTitleText(QStringLiteral("Time, s"));
    m_axisX->setLabelFormat("%.2f");

    m_axisY->setTitleText(QStringLiteral("Value"));
    m_axisY->setLabelFormat("%.2f");

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    const auto& sets = channelSets();
    for (const auto& set : sets) {
        m_combo->addItem(set.displayName);
    }

    connect(
        m_combo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &TelemetryChartWidget::onSelectionChanged
    );

    const int defaultIndex = findChannelSetIndex(defaultChannelSet);
    m_combo->setCurrentIndex(defaultIndex >= 0 ? defaultIndex : 0);

    rebuildSeries();
}

const QVector<TelemetryChartWidget::ChannelSetSpec>& TelemetryChartWidget::channelSets()
{
    static const QVector<ChannelSetSpec> sets = {
        {
            QStringLiteral("Ускорения (ax, ay, az)"),
            {
                {QStringLiteral("ax"), [](const DataPacket& p) { return static_cast<qreal>(p.ax); }},
                {QStringLiteral("ay"), [](const DataPacket& p) { return static_cast<qreal>(p.ay); }},
                {QStringLiteral("az"), [](const DataPacket& p) { return static_cast<qreal>(p.az); }},
            }
        },
        {
            QStringLiteral("Скорости (vx, vy, vz)"),
            {
                {QStringLiteral("vx"), [](const DataPacket& p) { return static_cast<qreal>(p.vx); }},
                {QStringLiteral("vy"), [](const DataPacket& p) { return static_cast<qreal>(p.vy); }},
                {QStringLiteral("vz"), [](const DataPacket& p) { return static_cast<qreal>(p.vz); }},
            }
        },
        {
            QStringLiteral("Координаты (sx, sy, sz)"),
            {
                {QStringLiteral("sx"), [](const DataPacket& p) { return static_cast<qreal>(p.sx); }},
                {QStringLiteral("sy"), [](const DataPacket& p) { return static_cast<qreal>(p.sy); }},
                {QStringLiteral("sz"), [](const DataPacket& p) { return static_cast<qreal>(p.sz); }},
            }
        },
        {
            QStringLiteral("Магнитометр (mx, my, mz)"),
            {
                {QStringLiteral("mx"), [](const DataPacket& p) { return static_cast<qreal>(p.mx); }},
                {QStringLiteral("my"), [](const DataPacket& p) { return static_cast<qreal>(p.my); }},
                {QStringLiteral("mz"), [](const DataPacket& p) { return static_cast<qreal>(p.mz); }},
            }
        },
        {
            QStringLiteral("Гироскоп (gx, gy, gz)"),
            {
                {QStringLiteral("gx"), [](const DataPacket& p) { return static_cast<qreal>(p.gx); }},
                {QStringLiteral("gy"), [](const DataPacket& p) { return static_cast<qreal>(p.gy); }},
                {QStringLiteral("gz"), [](const DataPacket& p) { return static_cast<qreal>(p.gz); }},
            }
        },
        {
            QStringLiteral("Углы (angx, angy, angz)"),
            {
                {QStringLiteral("angx"), [](const DataPacket& p) { return static_cast<qreal>(p.angx); }},
                {QStringLiteral("angy"), [](const DataPacket& p) { return static_cast<qreal>(p.angy); }},
                {QStringLiteral("angz"), [](const DataPacket& p) { return static_cast<qreal>(p.angz); }},
            }
        },
        {
            QStringLiteral("Скорость V"),
            {
                {QStringLiteral("V"), [](const DataPacket& p) { return static_cast<qreal>(p.V); }},
            }
        },
        {
            QStringLiteral("Направление H"),
            {
                {QStringLiteral("H"), [](const DataPacket& p) { return static_cast<qreal>(p.H); }},
            }
        },
        {
            QStringLiteral("Давление P"),
            {
                {QStringLiteral("P"), [](const DataPacket& p) { return static_cast<qreal>(p.P); }},
            }
        },
        {
            QStringLiteral("Температура T"),
            {
                {QStringLiteral("T"), [](const DataPacket& p) { return static_cast<qreal>(p.T); }},
            }
        },
        {
            QStringLiteral("Баро-высота BarAlt"),
            {
                {QStringLiteral("BarAlt"), [](const DataPacket& p) { return static_cast<qreal>(p.BarAlt); }},
            }
        },
        {
            QStringLiteral("Дистанция d"),
            {
                {QStringLiteral("d"), [](const DataPacket& p) { return static_cast<qreal>(p.d); }},
            }
        },
        {
            QStringLiteral("Tcycle"),
            {
                {QStringLiteral("Tcycle"), [](const DataPacket& p) { return static_cast<qreal>(p.Tcycle); }},
            }
        }
    };

    return sets;
}

int TelemetryChartWidget::findChannelSetIndex(const QString& name) const
{
    const auto& sets = channelSets();
    for (int i = 0; i < sets.size(); ++i) {
        if (sets[i].displayName == name) {
            return i;
        }
    }
    return -1;
}

void TelemetryChartWidget::rebuildSeries()
{
    for (auto* series : m_seriesList) {
        m_chart->removeSeries(series);
        series->deleteLater();
    }
    m_seriesList.clear();
    m_activeChannels.clear();

    const int selectedIndex = m_combo ? m_combo->currentIndex() : -1;
    const auto& sets = channelSets();

    if (selectedIndex < 0 || selectedIndex >= sets.size()) {
        resetViewport();
        return;
    }

    const auto& selectedSet = sets[selectedIndex];
    m_activeChannels = selectedSet.channels;

    const auto& palette = seriesPalette();

    for (int i = 0; i < m_activeChannels.size(); ++i) {
        auto* series = new QLineSeries();
        series->setName(m_activeChannels[i].seriesName);

        QPen pen(palette[i % palette.size()]);
        pen.setWidthF(2.0);
        series->setPen(pen);

        m_chart->addSeries(series);
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);

        m_seriesList.append(series);
    }

    resetViewport();
}

void TelemetryChartWidget::resetViewport()
{
    m_lastX = 0.0;
    m_hasData = false;

    m_axisX->setRange(0.0, m_timeWindowSec);
    m_axisY->setRange(-1.0, 1.0);
}

void TelemetryChartWidget::appendPacket(const DataPacket& packet)
{
    if (m_activeChannels.isEmpty() || m_seriesList.isEmpty()) {
        return;
    }

    const qreal x = static_cast<qreal>(packet.time) / 1000.0;

    for (int i = 0; i < m_seriesList.size() && i < m_activeChannels.size(); ++i) {
        const qreal y = m_activeChannels[i].getter(packet);
        m_seriesList[i]->append(x, y);
    }

    m_lastX = x;
    m_hasData = true;

    trimSeriesIfNeeded();
    updateXAxis();
    updateYAxis();
}

void TelemetryChartWidget::clear()
{
    for (auto* series : m_seriesList) {
        series->clear();
    }
    resetViewport();
}

void TelemetryChartWidget::setSelectedChannelSet(const QString& channelSetName)
{
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

QString TelemetryChartWidget::selectedChannelSet() const
{
    return m_combo ? m_combo->currentText() : QString();
}

void TelemetryChartWidget::setTimeWindow(qreal seconds)
{
    m_timeWindowSec = std::max<qreal>(0.1, seconds);
    updateXAxis();
}

void TelemetryChartWidget::setPointLimit(int limit)
{
    m_pointLimit = std::max(2, limit);
    trimSeriesIfNeeded();
    updateYAxis();
}

void TelemetryChartWidget::onSelectionChanged(int)
{
    rebuildSeries();
}

void TelemetryChartWidget::updateXAxis()
{
    if (!m_hasData) {
        m_axisX->setRange(0.0, m_timeWindowSec);
        return;
    }

    const qreal maxX = std::max<qreal>(m_lastX, 0.001);
    const qreal minX = std::max<qreal>(0.0, maxX - m_timeWindowSec);

    if (maxX <= minX) {
        m_axisX->setRange(minX, minX + 0.001);
        return;
    }

    m_axisX->setRange(minX, maxX);
}

void TelemetryChartWidget::updateYAxis()
{
    if (m_seriesList.isEmpty()) {
        m_axisY->setRange(-1.0, 1.0);
        return;
    }

    bool haveData = false;
    qreal minY = 0.0;
    qreal maxY = 0.0;

    for (const auto* series : m_seriesList) {
        const auto points = series->points();
        for (const auto& point : points) {
            const qreal y = point.y();

            if (!haveData) {
                minY = maxY = y;
                haveData = true;
            } else {
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (!haveData) {
        m_axisY->setRange(-1.0, 1.0);
        return;
    }

    const qreal span = maxY - minY;
    qreal padding = 0.0;

    if (span <= 0.000001) {
        padding = std::max<qreal>(0.5, std::abs(maxY) * 0.1);
    } else {
        padding = std::max<qreal>(span * 0.10, 0.5);
    }

    m_axisY->setRange(minY - padding, maxY + padding);
}

void TelemetryChartWidget::trimSeriesIfNeeded()
{
    if (m_seriesList.isEmpty()) {
        return;
    }

    const int count = m_seriesList.first()->count();
    if (count <= m_pointLimit) {
        return;
    }

    const int removeCount = count - m_pointLimit;

    for (auto* series : m_seriesList) {
        series->removePoints(0, removeCount);
    }
}
