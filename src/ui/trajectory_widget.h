// ui/trajectory_widget.h
#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include "datasource/telemetry_protocol.h"

class TrajectoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrajectoryWidget(QWidget* parent = nullptr);

public slots:
    void updatePosition(const DataPacket& packet);
    void clearTrajectory();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawTrajectory(QPainter& painter);
    void drawDronePosition(QPainter& painter, const QPointF& pos, float heading);
    void drawGrid(QPainter& painter);
    void drawLabels(QPainter& painter);
    void calculateBounds();
    QPointF mapToWidget(const QPointF& point);

    QVector<QPointF> m_trajectory;
    QPointF m_currentPos{0, 0};
    float m_heading{0};
    
    float m_minX{-10}, m_maxX{10};
    float m_minY{-10}, m_maxY{10};
    float m_scale{1.0};
    
    static constexpr int MAX_TRAIL_POINTS = 2000;
    bool m_autoScale{true};
    float m_margin{40};
};
