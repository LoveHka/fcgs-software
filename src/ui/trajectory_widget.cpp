// ui/trajectory_widget.cpp

#include "ui/trajectory_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QtMath>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TrajectoryWidget::TrajectoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(250, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrajectoryWidget::updatePosition(const DataPacket& packet)
{
    m_currentPos = QPointF(packet.sx, packet.sy);
    m_heading = packet.angz;

    m_trajectory.append(m_currentPos);
    if (m_trajectory.size() > MAX_TRAIL_POINTS) {
        m_trajectory.removeFirst();
    }

    if (m_autoScale) {
        calculateBounds();
    }

    update();
}

void TrajectoryWidget::clearTrajectory()
{
    m_trajectory.clear();
    m_minX = -10;
    m_maxX = 10;
    m_minY = -10;
    m_maxY = 10;
    update();
}

void TrajectoryWidget::calculateBounds()
{
    if (m_trajectory.isEmpty()) {
        m_minX = -10;
        m_maxX = 10;
        m_minY = -10;
        m_maxY = 10;
        return;
    }

    m_minX = m_currentPos.x();
    m_maxX = m_currentPos.x();
    m_minY = m_currentPos.y();
    m_maxY = m_currentPos.y();

    for (const auto& point : m_trajectory) {
        m_minX = std::min(m_minX, static_cast<float>(point.x()));
        m_maxX = std::max(m_maxX, static_cast<float>(point.x()));
        m_minY = std::min(m_minY, static_cast<float>(point.y()));
        m_maxY = std::max(m_maxY, static_cast<float>(point.y()));
    }

    // Add margin to bounds
    float rangeX = m_maxX - m_minX;
    float rangeY = m_maxY - m_minY;
    
    // Minimum range of 10 units
    float minRange = 10.0f;
    rangeX = std::max(rangeX, minRange);
    rangeY = std::max(rangeY, minRange);

    float centerX = (m_minX + m_maxX) / 2.0f;
    float centerY = (m_minY + m_maxY) / 2.0f;

    float halfRange = std::max(rangeX, rangeY) / 2.0f * 1.2f; // 20% extra margin

    m_minX = centerX - halfRange;
    m_maxX = centerX + halfRange;
    m_minY = centerY - halfRange;
    m_maxY = centerY + halfRange;
}

QPointF TrajectoryWidget::mapToWidget(const QPointF& point)
{
    float drawWidth = width() - 2 * m_margin;
    float drawHeight = height() - 2 * m_margin;

    float x = m_margin + (point.x() - m_minX) / (m_maxX - m_minX) * drawWidth;
    float y = height() - m_margin - (point.y() - m_minY) / (m_maxY - m_minY) * drawHeight;

    return QPointF(x, y);
}

void TrajectoryWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(0x1a, 0x1a, 0x2e));

    // Border
    painter.setPen(QPen(QColor(0x40, 0x40, 0x60), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    calculateBounds();
    drawGrid(painter);
    drawTrajectory(painter);
    drawDronePosition(painter, m_currentPos, m_heading);
    drawLabels(painter);
}

void TrajectoryWidget::drawGrid(QPainter& painter)
{
    float drawWidth = width() - 2 * m_margin;
    float drawHeight = height() - 2 * m_margin;

    // Calculate grid spacing
    float rangeX = m_maxX - m_minX;
    float rangeY = m_maxY - m_minY;
    float range = std::max(rangeX, rangeY);

    // Choose grid step based on scale
    float gridStep = 1.0f;
    if (range > 1000) gridStep = 100.0f;
    else if (range > 100) gridStep = 10.0f;
    else if (range > 10) gridStep = 5.0f;
    else gridStep = 1.0f;

    // Draw grid lines
    painter.setPen(QPen(QColor(0x30, 0x30, 0x50), 1, Qt::DotLine));

    // Vertical lines
    float startX = std::floor(m_minX / gridStep) * gridStep;
    for (float x = startX; x <= m_maxX; x += gridStep) {
        QPointF p1 = mapToWidget(QPointF(x, m_minY));
        QPointF p2 = mapToWidget(QPointF(x, m_maxY));
        painter.drawLine(p1, p2);
    }

    // Horizontal lines
    float startY = std::floor(m_minY / gridStep) * gridStep;
    for (float y = startY; y <= m_maxY; y += gridStep) {
        QPointF p1 = mapToWidget(QPointF(m_minX, y));
        QPointF p2 = mapToWidget(QPointF(m_maxX, y));
        painter.drawLine(p1, p2);
    }

    // Draw coordinate labels on grid
    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QColor(0x80, 0x80, 0xa0));
    
    for (float x = startX; x <= m_maxX; x += gridStep) {
        QPointF pos = mapToWidget(QPointF(x, m_minY));
        QString label = QString::number(x, 'f', 1);
        painter.drawText(pos.x() - 15, height() - m_margin + 15, label);
    }

    for (float y = startY; y <= m_maxY; y += gridStep) {
        QPointF pos = mapToWidget(QPointF(m_minX, y));
        QString label = QString::number(y, 'f', 1);
        painter.drawText(m_margin - 35, pos.y() + 4, label);
    }
}

void TrajectoryWidget::drawTrajectory(QPainter& painter)
{
    if (m_trajectory.size() < 2) {
        return;
    }

    // Draw trajectory line
    QPainterPath path;
    QPointF firstPoint = mapToWidget(m_trajectory.first());
    path.moveTo(firstPoint);

    for (int i = 1; i < m_trajectory.size(); ++i) {
        QPointF point = mapToWidget(m_trajectory[i]);
        path.lineTo(point);
    }

    // Gradient color for trajectory
    QPen trailPen(QColor(0x1f, 0x77, 0xb4), 2.0);
    trailPen.setCapStyle(Qt::RoundCap);
    trailPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(trailPen);
    painter.drawPath(path);

    // Draw faded trail dots
    if (m_trajectory.size() > 10) {
        int dotCount = std::min(50, (int)m_trajectory.size());
        for (int i = 0; i < dotCount; ++i) {
            int idx = m_trajectory.size() - 1 - i;
            if (idx < 0) break;
            
            QPointF point = mapToWidget(m_trajectory[idx]);
            float alpha = 0.3f * (1.0f - (float)i / dotCount);
            painter.setPen(QPen(QColor(0x1f, 0x77, 0xb4, (int)(alpha * 255)), 3));
            painter.drawPoint(point);
        }
    }
}

void TrajectoryWidget::drawDronePosition(QPainter& painter, const QPointF& pos, float heading)
{
    QPointF center = mapToWidget(pos);
    
    // Draw drone icon as triangle pointing in heading direction
    float size = 12.0f;
    float angleRad = qDegreesToRadians(heading);
    
    // Calculate triangle points
    QVector<QPointF> triangle;
    for (int i = 0; i < 3; ++i) {
        float angle = angleRad - M_PI_2 + (i * 2.0f * M_PI / 3.0f);
        triangle.append(QPointF(
            center.x() + size * qCos(angle),
            center.y() - size * qSin(angle)
        ));
    }

    // Draw drone body
    QBrush droneBrush(QColor(0xff, 0x7f, 0x0e));
    QPen dronePen(QColor(0xff, 0x9f, 0x3f), 2);
    painter.setPen(dronePen);
    painter.setBrush(droneBrush);
    painter.drawPolygon(triangle);

    // Draw heading line
    float lineLength = size * 2;
    QPointF lineEnd(
        center.x() + lineLength * qCos(angleRad),
        center.y() - lineLength * qSin(angleRad)
    );
    
    QPen linePen(QColor(0xff, 0x7f, 0x0e), 2);
    painter.setPen(linePen);
    painter.drawLine(center, lineEnd);

    // Draw position marker circle
    QPen circlePen(QColor(0xff, 0x7f, 0x0e, 100), 1);
    painter.setPen(circlePen);
    painter.drawEllipse(center, size * 1.5f, size * 1.5f);
}

void TrajectoryWidget::drawLabels(QPainter& painter)
{
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(QColor(0xff, 0xff, 0xff));
    
    // Title
    painter.drawText(m_margin, 20, QStringLiteral("Траектория (вид сверху)"));

    // Current position info
    if (!m_trajectory.isEmpty()) {
        painter.setFont(QFont("Consolas", 9));
        painter.setPen(QColor(0x1f, 0xbe, 0xcf));
        
        QString info = QString("X: %1  Y: %2")
            .arg(m_currentPos.x(), 8, 'f', 2)
            .arg(m_currentPos.y(), 8, 'f', 2);
        painter.drawText(m_margin, height() - 10, info);
    }
}
