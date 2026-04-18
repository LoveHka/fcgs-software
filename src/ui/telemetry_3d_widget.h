// ui/telemetry_3d_widget.h
#pragma once

#include <QWidget>
#include <QVector4D>
#include "datasource/telemetry_protocol.h"

class Telemetry3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Telemetry3DWidget(QWidget* parent = nullptr);

public slots:
    void updateOrientation(const DataPacket& packet);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector3D project3D(const QVector3D& point, const QMatrix4x4& mvp) const;
    void drawLine3D(QPainter& painter, const QVector3D& p1, const QVector3D& p2,
                    const QMatrix4x4& mvp, const QPen& pen);
    void drawLineScreen(QPainter& painter, const QPointF& p1, const QPointF& p2, const QPen& pen);
    void drawQuad3D(QPainter& painter, const QVector3D& p1, const QVector3D& p2,
                    const QVector3D& p3, const QVector3D& p4,
                    const QMatrix4x4& mvp, const QBrush& brush, const QPen& pen);
    void drawCircle3D(QPainter& painter, const QVector3D& center, float radius,
                      const QMatrix4x4& mvp, const QBrush& brush);
    void drawAirplane(QPainter& painter, const QMatrix4x4& model, const QMatrix4x4& mvp);
    void drawGroundPlane(QPainter& painter, const QMatrix4x4& mvp);
    void drawTrail(QPainter& painter, const QMatrix4x4& viewProjNoTranslate);
    void drawWorldAxes(QPainter& painter, const QMatrix4x4& mvp);
    void drawVelocityVector(QPainter& painter, const QMatrix4x4& model, const QMatrix4x4& mvp);
    void drawCompassAxes(QPainter& painter, const QMatrix4x4& proj, const QMatrix4x4& view);

    QVector3D m_position{0, 0, 0};
    QVector3D m_velocity{0, 0, 0};
    float m_roll{0};
    float m_pitch{0};
    float m_yaw{0};

    QVector<QVector3D> m_trail;
    static constexpr int MAX_TRAIL_POINTS = 200;

    float m_cameraDistance{5.0f};
    float m_cameraAngleX{25.0f};
    float m_cameraAngleY{45.0f};

    bool m_isDragging{false};
    QPoint m_lastMousePos;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};
