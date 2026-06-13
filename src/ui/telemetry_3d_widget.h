// ui/telemetry_3d_widget.h
#pragma once

#include "datasource/telemetry_protocol.h"
#include <QMatrix4x4>
#include <QPoint>
#include <QTimer>
#include <QVector3D>
#include <QVector4D>
#include <QVector>
#include <QWidget>

// Предварительные объявления классов Qt для ускорения компиляции
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QPen;
class QBrush;

class Telemetry3DWidget : public QWidget {
  Q_OBJECT

public:
  explicit Telemetry3DWidget(QWidget *parent = nullptr);
  ~Telemetry3DWidget() override = default;

public slots:
  void updateOrientation(const DataPacket &packet);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  QVector3D project3D(const QVector3D &point, const QMatrix4x4 &mvp) const;

  void drawLine3D(QPainter &painter, const QVector3D &p1, const QVector3D &p2,
                  const QMatrix4x4 &mvp, const QPen &pen);

  void drawLineScreen(QPainter &painter, const QPointF &p1, const QPointF &p2,
                      const QPen &pen);

  void drawQuad3D(QPainter &painter, const QVector3D &p1, const QVector3D &p2,
                  const QVector3D &p3, const QVector3D &p4,
                  const QMatrix4x4 &mvp, const QBrush &brush, const QPen &pen);

  void drawCircle3D(QPainter &painter, const QVector3D &center, float radius,
                    const QMatrix4x4 &mvp, const QBrush &brush);

  void drawAirplane(QPainter &painter, const QMatrix4x4 &model,
                    const QMatrix4x4 &mvp);

  void drawGroundPlane(QPainter &painter, const QMatrix4x4 &mvp);
  void drawTrail(QPainter &painter, const QMatrix4x4 &viewProjNoTranslate);
  void drawWorldAxes(QPainter &painter, const QMatrix4x4 &mvp);

  void drawVelocityVector(QPainter &painter, const QMatrix4x4 &model,
                          const QMatrix4x4 &mvp);

  void drawCompassAxes(QPainter &painter, const QMatrix4x4 &proj,
                       const QMatrix4x4 &view);

  // Данные телеметрии
  QVector3D m_position{0.0f, 0.0f, 0.0f};
  QVector3D m_velocity{0.0f, 0.0f, 0.0f};
  float m_roll{0.0f};
  float m_pitch{0.0f};
  float m_yaw{0.0f};

  // Хвост (траектория) движения
  QVector<QVector3D> m_trail;
  static constexpr int MAX_TRAIL_POINTS = 200;

  // Параметры камеры
  float m_cameraDistance{5.0f};
  float m_cameraAngleX{25.0f};
  float m_cameraAngleY{45.0f};

  // Состояние мыши для вращения камеры
  bool m_isDragging{false};
  QPoint m_lastMousePos;

  // Таймер для обновления сцены
  QTimer *renderTimer = nullptr;
};
