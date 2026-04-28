// ui/telemetry_3d_widget.cpp

#include "ui/telemetry_3d_widget.h"

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// World coordinate system:
//   X — forward/back (horizontal)
//   Y — left/right  (horizontal)
//   Z — UP (altitude)
//
// Rotations (from FC):
//   angx (roll)  — rotation around X axis
//   angy (pitch) — rotation around Y axis
//   angz (yaw)   — rotation around Z axis (vertical)

Telemetry3DWidget::Telemetry3DWidget(QWidget *parent) : QWidget(parent) {
  setMinimumSize(300, 250);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  renderTimer = new QTimer(this);

  // Соединяем сигнал таймера со стандартным слотом update()
  connect(renderTimer, &QTimer::timeout, this,
          QOverload<>::of(&Telemetry3DWidget::update));

  renderTimer->start(33); // 30 раз в секунду
}

QVector3D Telemetry3DWidget::project3D(const QVector3D &point,
                                       const QMatrix4x4 &mvp) const {
  QVector4D clip = mvp * QVector4D(point, 1.0f);
  if (qAbs(clip.w()) < 0.0001f)
    return QVector3D(0, 0, -1);
  QVector3D ndc = clip.toVector3D() / clip.w();

  float halfW = width() / 2.0f;
  float halfH = height() / 2.0f;

  return QVector3D(halfW + ndc.x() * halfW, halfH - ndc.y() * halfH, ndc.z());
}

void Telemetry3DWidget::drawLine3D(QPainter &painter, const QVector3D &p1,
                                   const QVector3D &p2, const QMatrix4x4 &mvp,
                                   const QPen &pen) {
  QVector3D sp1 = project3D(p1, mvp);
  QVector3D sp2 = project3D(p2, mvp);

  if (sp1.z() < -1 || sp2.z() < -1)
    return;

  painter.setPen(pen);
  painter.drawLine(QPointF(sp1.x(), sp1.y()), QPointF(sp2.x(), sp2.y()));
}

void Telemetry3DWidget::drawLineScreen(QPainter &painter, const QPointF &p1,
                                       const QPointF &p2, const QPen &pen) {
  painter.setPen(pen);
  painter.drawLine(p1, p2);
}

void Telemetry3DWidget::drawQuad3D(QPainter &painter, const QVector3D &p1,
                                   const QVector3D &p2, const QVector3D &p3,
                                   const QVector3D &p4, const QMatrix4x4 &mvp,
                                   const QBrush &brush, const QPen &pen) {
  QVector3D pts[4] = {project3D(p1, mvp), project3D(p2, mvp),
                      project3D(p3, mvp), project3D(p4, mvp)};

  for (const auto &p : pts) {
    if (p.z() < -1)
      return;
  }

  QPolygonF polygon;
  for (const auto &p : pts) {
    polygon << QPointF(p.x(), p.y());
  }

  painter.setBrush(brush);
  painter.setPen(pen);
  painter.drawPolygon(polygon);
}

void Telemetry3DWidget::drawCircle3D(QPainter &painter, const QVector3D &center,
                                     float radius, const QMatrix4x4 &mvp,
                                     const QBrush &brush) {
  QVector3D projected = project3D(center, mvp);
  QVector3D edge = project3D(center + QVector3D(radius, 0, 0), mvp);

  if (projected.z() < -1)
    return;

  float projectedRadius =
      QLineF(projected.toPointF(), edge.toPointF()).length();

  painter.setBrush(brush);
  painter.setPen(Qt::NoPen);
  painter.drawEllipse(projected.toPointF(), projectedRadius, projectedRadius);
}

void Telemetry3DWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Background
  painter.fillRect(rect(), QColor(0x12, 0x12, 0x1e));

  // Border
  painter.setPen(QPen(QColor(0x40, 0x40, 0x60), 1));
  painter.drawRect(rect().adjusted(0, 0, -1, -1));

  // === Camera ===
  // World: Z is UP. Camera orbits around origin.
  // cameraAngleX = elevation from horizontal plane (0 = horizon, 90 = top)
  // cameraAngleY = azimuth angle in XY plane
  QMatrix4x4 projection;
  projection.setToIdentity();
  float aspect = float(width()) / float(height());
  projection.perspective(45.0f, aspect, 0.1f, 100.0f);

  QMatrix4x4 view;
  view.setToIdentity();

  // Spherical coordinates: XY is horizontal plane, Z is up
  float elevRad = qDegreesToRadians(m_cameraAngleX);
  float azimRad = qDegreesToRadians(m_cameraAngleY);

  float horizontalDist = m_cameraDistance * qCos(elevRad);
  float camX = horizontalDist * qCos(azimRad);
  float camY = horizontalDist * qSin(azimRad);
  float camZ = m_cameraDistance * qSin(elevRad);

  // UP vector is always +Z
  view.lookAt(QVector3D(camX, camY, camZ), QVector3D(0, 0, 0),
              QVector3D(0, 0, 1));

  QMatrix4x4 mvp = projection * view;

  // === Ground plane (at Z = aircraft_altitude - 2) ===
  // drawGroundPlane(painter, mvp);

  // === Trail === Удалил, нафиг оно не нужно сейчас нам )))
  // drawTrail(painter, mvp);

  // === Aircraft model ===
  // Airplane: nose points +X, wings along ±Y, up is +Z
  // Rotation order from FC: yaw(angz) around Z, then pitch(angy) around Y, then
  // roll(angx) around X
  QMatrix4x4 model;
  model.rotate(-m_yaw, 0, 0, 1);   // yaw around Z
  model.rotate(-m_pitch, 0, 1, 0); // pitch around Y
  model.rotate(m_roll, 1, 0, 0);   // roll around X

  drawAirplane(painter, model, mvp);
  drawVelocityVector(painter, model, mvp);

  // === World axes compass (bottom-left) — projected through current view ===
  drawCompassAxes(painter, projection, view);

  // === Title ===
  painter.setFont(QFont("Arial", 10, QFont::Bold));
  painter.setPen(QColor(0xff, 0xff, 0xff));
  painter.drawText(15, 25, QStringLiteral("3D Позиция БПЛА"));

  // === Info line ===
  painter.setFont(QFont("Consolas", 9));
  painter.setPen(QColor(0x1f, 0xbe, 0xcf));
  QString info =
      QString("X:%1  Y:%2  Z:%3    |V|:%4  angX:%5  angY:%6  angZ:%7")
          .arg(m_position.x(), 7, 'f', 2)
          .arg(m_position.y(), 7, 'f', 2)
          .arg(m_position.z(), 7, 'f', 2)
          .arg(m_velocity.length(), 5, 'f', 2)
          .arg(m_roll, 5, 'f', 1)
          .arg(m_pitch, 5, 'f', 1)
          .arg(m_yaw, 5, 'f', 1);
  painter.drawText(15, height() - 15, info);
}

void Telemetry3DWidget::drawGroundPlane(QPainter &painter,
                                        const QMatrix4x4 &mvp) {
  float gridSize = 10.0f;
  float gridStep = 1.0f;
  float groundZ = -2.0f; // 2 meters below aircraft

  QPen gridPen(QColor(0x30, 0x30, 0x50), 1);

  for (float i = -gridSize; i <= gridSize; i += gridStep) {
    drawLine3D(painter, QVector3D(i, -gridSize, groundZ),
               QVector3D(i, gridSize, groundZ), mvp, gridPen);
    drawLine3D(painter, QVector3D(-gridSize, i, groundZ),
               QVector3D(gridSize, i, groundZ), mvp, gridPen);
  }
}

void Telemetry3DWidget::drawTrail(QPainter &painter, const QMatrix4x4 &mvp) {
  if (m_trail.size() < 2)
    return;

  for (int i = 1; i < m_trail.size(); ++i) {
    QVector3D relPrev = m_trail[i - 1] - m_position;
    QVector3D relCurr = m_trail[i] - m_position;

    float alpha = 0.2f + 0.8f * (float(i) / m_trail.size());
    QPen trailPen(QColor(0x1f, 0x77, 0xb4, (int)(alpha * 255)), 2);
    drawLine3D(painter, relPrev, relCurr, mvp, trailPen);
  }
}

void Telemetry3DWidget::drawAirplane(QPainter &painter, const QMatrix4x4 &model,
                                     const QMatrix4x4 &mvp) {
  QMatrix4x4 mvp2 = mvp * model;

  QBrush fuselageBrush(QColor(180, 180, 190));
  QBrush wingBrush(QColor(140, 140, 160));
  QBrush tailBrush(QColor(150, 150, 170));
  QPen outlinePen(QColor(200, 200, 210));

  // === FUSELAGE ===
  // Nose along +X, wings along ±Y, up along +Z
  float fLen = 0.8f;
  float fWidth = 0.12f;
  float fHeight = 0.1f;

  // Top face
  drawQuad3D(painter, QVector3D(-fLen * 0.5f, -fWidth, fHeight),
             QVector3D(fLen * 0.5f, -fWidth, fHeight),
             QVector3D(fLen * 0.5f, fWidth, fHeight),
             QVector3D(-fLen * 0.5f, fWidth, fHeight), mvp2, fuselageBrush,
             outlinePen);
  // Bottom face
  drawQuad3D(painter, QVector3D(-fLen * 0.5f, -fWidth, -fHeight),
             QVector3D(-fLen * 0.5f, fWidth, -fHeight),
             QVector3D(fLen * 0.5f, fWidth, -fHeight),
             QVector3D(fLen * 0.5f, -fWidth, -fHeight), mvp2, fuselageBrush,
             outlinePen);
  // Nose face
  QBrush noseBrush(QColor(200, 60, 60));
  drawQuad3D(painter, QVector3D(fLen * 0.5f, -fWidth, -fHeight),
             QVector3D(fLen * 0.5f, fWidth, -fHeight),
             QVector3D(fLen * 0.5f, fWidth, fHeight),
             QVector3D(fLen * 0.5f, -fWidth, fHeight), mvp2, noseBrush,
             outlinePen);
  // Tail face
  drawQuad3D(painter, QVector3D(-fLen * 0.5f, -fWidth, -fHeight),
             QVector3D(-fLen * 0.5f, -fWidth, fHeight),
             QVector3D(-fLen * 0.5f, fWidth, fHeight),
             QVector3D(-fLen * 0.5f, fWidth, -fHeight), mvp2, fuselageBrush,
             outlinePen);
  // Side faces
  drawQuad3D(painter, QVector3D(-fLen * 0.5f, -fWidth, -fHeight),
             QVector3D(fLen * 0.5f, -fWidth, -fHeight),
             QVector3D(fLen * 0.5f, -fWidth, fHeight),
             QVector3D(-fLen * 0.5f, -fWidth, fHeight), mvp2, fuselageBrush,
             outlinePen);
  drawQuad3D(painter, QVector3D(-fLen * 0.5f, fWidth, -fHeight),
             QVector3D(-fLen * 0.5f, fWidth, fHeight),
             QVector3D(fLen * 0.5f, fWidth, fHeight),
             QVector3D(fLen * 0.5f, fWidth, -fHeight), mvp2, fuselageBrush,
             outlinePen);

  // === MAIN WINGS ===
  // Wings extend along ±Y, chord along X
  float wingSpan = 1.6f;
  float wingChord = 0.18f;
  float wingThick = 0.025f;
  float wingZ = 0.0f;
  float wingXCenter = 0.0f;

  // Top wing face
  drawQuad3D(painter,
             QVector3D(wingXCenter - wingChord * 0.5f, -wingSpan * 0.5f,
                       wingZ + wingThick),
             QVector3D(wingXCenter + wingChord * 0.5f, -wingSpan * 0.5f,
                       wingZ + wingThick),
             QVector3D(wingXCenter + wingChord * 0.5f, wingSpan * 0.5f,
                       wingZ + wingThick),
             QVector3D(wingXCenter - wingChord * 0.5f, wingSpan * 0.5f,
                       wingZ + wingThick),
             mvp2, wingBrush, outlinePen);
  // Bottom wing face
  drawQuad3D(painter,
             QVector3D(wingXCenter - wingChord * 0.5f, -wingSpan * 0.5f,
                       wingZ - wingThick),
             QVector3D(wingXCenter - wingChord * 0.5f, wingSpan * 0.5f,
                       wingZ - wingThick),
             QVector3D(wingXCenter + wingChord * 0.5f, wingSpan * 0.5f,
                       wingZ - wingThick),
             QVector3D(wingXCenter + wingChord * 0.5f, -wingSpan * 0.5f,
                       wingZ - wingThick),
             mvp2, wingBrush, outlinePen);

  // === HORIZONTAL STABILIZER ===
  float hStabSpan = 0.5f;
  float hStabChord = 0.12f;
  float hStabX = -fLen * 0.42f;
  float hStabZ = 0.0f;

  drawQuad3D(painter,
             QVector3D(hStabX - hStabChord * 0.5f, -hStabSpan * 0.5f,
                       hStabZ + wingThick),
             QVector3D(hStabX + hStabChord * 0.5f, -hStabSpan * 0.5f,
                       hStabZ + wingThick),
             QVector3D(hStabX + hStabChord * 0.5f, hStabSpan * 0.5f,
                       hStabZ + wingThick),
             QVector3D(hStabX - hStabChord * 0.5f, hStabSpan * 0.5f,
                       hStabZ + wingThick),
             mvp2, tailBrush, outlinePen);

  // === VERTICAL STABILIZER (fin) ===
  // Fin goes UP from fuselage (+Z), chord along X
  float vStabHeight = 0.3f;
  float vStabChord = 0.15f;
  float vStabX = -fLen * 0.42f;

  drawQuad3D(painter, QVector3D(vStabX - vStabChord * 0.5f, 0.005f, 0.0f),
             QVector3D(vStabX + vStabChord * 0.5f, 0.005f, 0.0f),
             QVector3D(vStabX + vStabChord * 0.5f, 0.005f, vStabHeight),
             QVector3D(vStabX - vStabChord * 0.5f, 0.005f, vStabHeight), mvp2,
             tailBrush, outlinePen);
  // Other side of fin
  drawQuad3D(painter, QVector3D(vStabX - vStabChord * 0.5f, -0.005f, 0.0f),
             QVector3D(vStabX - vStabChord * 0.5f, -0.005f, vStabHeight),
             QVector3D(vStabX + vStabChord * 0.5f, -0.005f, vStabHeight),
             QVector3D(vStabX + vStabChord * 0.5f, -0.005f, 0.0f), mvp2,
             tailBrush, outlinePen);
  // Top edge
  drawQuad3D(painter,
             QVector3D(vStabX - vStabChord * 0.5f, -0.005f, vStabHeight),
             QVector3D(vStabX - vStabChord * 0.5f, 0.005f, vStabHeight),
             QVector3D(vStabX + vStabChord * 0.5f, 0.005f, vStabHeight),
             QVector3D(vStabX + vStabChord * 0.5f, -0.005f, vStabHeight), mvp2,
             tailBrush, outlinePen);

  // === COCKPIT ===
  QBrush cockpitBrush(QColor(80, 150, 220, 150));
  float cockpitX = fLen * 0.15f;
  float cockpitLen = 0.15f;
  float cockpitH = 0.07f;

  drawQuad3D(
      painter, QVector3D(cockpitX, -fWidth * 0.5f, fHeight),
      QVector3D(cockpitX + cockpitLen, -fWidth * 0.4f, fHeight + cockpitH),
      QVector3D(cockpitX + cockpitLen, fWidth * 0.4f, fHeight + cockpitH),
      QVector3D(cockpitX, fWidth * 0.5f, fHeight), mvp2, cockpitBrush,
      QPen(QColor(100, 180, 255, 180)));

  // === BODY AXES ===
  QPen xPen(QColor(255, 60, 60), 2);
  QPen yPen(QColor(60, 255, 60), 2);
  QPen zPen(QColor(60, 60, 255), 2);
  float axisLen = 0.6f;
  // X — forward (red)
  drawLine3D(painter, QVector3D(0, 0, 0), QVector3D(axisLen, 0, 0), mvp2, xPen);
  // Y — right wing (green)
  drawLine3D(painter, QVector3D(0, 0, 0), QVector3D(0, axisLen, 0), mvp2, yPen);
  // Z — up (blue)
  drawLine3D(painter, QVector3D(0, 0, 0), QVector3D(0, 0, axisLen), mvp2, zPen);
}

void Telemetry3DWidget::drawVelocityVector(QPainter &painter,
                                           const QMatrix4x4 &model,
                                           const QMatrix4x4 &mvp) {
  float vScale = 0.3f;
  QVector3D vel = m_velocity * vScale;

  if (vel.length() < 0.01f)
    return;

  QPen velPen(QColor(255, 200, 0), 3);
  velPen.setCapStyle(Qt::RoundCap);

  drawLine3D(painter, QVector3D(0, 0, 0), vel, mvp, velPen);

  // Arrowhead
  QVector3D dir = vel.normalized();
  float tipLen = 0.15f;
  QVector3D tip = vel;

  // Find perpendicular vectors
  QVector3D ref = (qAbs(QVector3D::dotProduct(dir, QVector3D(0, 0, 1))) > 0.9f)
                      ? QVector3D(1, 0, 0)
                      : QVector3D(0, 0, 1);
  QVector3D perp1 =
      QVector3D::crossProduct(dir, ref).normalized() * tipLen * 0.5f;
  QVector3D perp2 =
      QVector3D::crossProduct(dir, perp1).normalized() * tipLen * 0.5f;

  QVector3D base = tip - dir * tipLen;
  QBrush arrowBrush(QColor(255, 200, 0));

  drawQuad3D(painter, tip, base + perp1, base, tip, mvp, arrowBrush, Qt::NoPen);
  drawQuad3D(painter, tip, base, base - perp1, tip, mvp, arrowBrush, Qt::NoPen);
  drawQuad3D(painter, tip, base + perp2, base, tip, mvp, arrowBrush, Qt::NoPen);
  drawQuad3D(painter, tip, base, base - perp2, tip, mvp, arrowBrush, Qt::NoPen);
}

void Telemetry3DWidget::drawCompassAxes(QPainter &painter,
                                        const QMatrix4x4 &proj,
                                        const QMatrix4x4 &view) {
  // Draw world axes as seen from current camera angle
  int cx = 65, cy = height() - 65;
  float len3D = 1.0f;

  // Project world axis directions through the camera
  // We project two points: origin and tip of each axis
  // Then compute screen-space direction from center
  auto projectDir = [&](const QVector3D &dir) -> QPointF {
    // Project origin
    QVector4D o = (proj * view) * QVector4D(0, 0, 0, 1);
    // Project tip
    QVector4D t = (proj * view) * QVector4D(dir * len3D, 1.0f);
    if (qAbs(o.w()) < 0.0001f || qAbs(t.w()) < 0.0001f)
      return QPointF(0, 0);
    QVector3D no = o.toVector3D() / o.w();
    QVector3D nt = t.toVector3D() / t.w();

    float halfW = width() / 2.0f;
    float halfH = height() / 2.0f;

    QPointF screenO(halfW + no.x() * halfW, halfH - no.y() * halfH);
    QPointF screenT(halfW + nt.x() * halfW, halfH - nt.y() * halfH);

    // Direction from center on screen
    return screenT - screenO;
  };

  QPointF xDir = projectDir(QVector3D(1, 0, 0));
  QPointF yDir = projectDir(QVector3D(0, 1, 0));
  QPointF zDir = projectDir(QVector3D(0, 0, 1));

  float xScreenLen =
      std::max(1.0f, (float)qSqrt(xDir.x() * xDir.x() + xDir.y() * xDir.y()));
  float yScreenLen =
      std::max(1.0f, (float)qSqrt(yDir.x() * yDir.x() + yDir.y() * yDir.y()));
  float zScreenLen =
      std::max(1.0f, (float)qSqrt(zDir.x() * zDir.x() + zDir.y() * zDir.y()));

  float drawLen = 35.0f;

  // Background
  painter.setBrush(QBrush(QColor(0x1a, 0x1a, 0x2e, 220)));
  painter.setPen(QPen(QColor(0x50, 0x50, 0x70), 1));
  painter.drawEllipse(QPointF(cx, cy), 48, 48);

  painter.setFont(QFont("Arial", 7));
  painter.setPen(QColor(0x88, 0x88, 0xaa));
  painter.drawText(cx - 12, cy + 48, QStringLiteral("Мир"));

  QPen xPen(QColor(255, 80, 80), 2);
  QPen yPen(QColor(80, 255, 80), 2);
  QPen zPen(QColor(80, 130, 255), 2);

  // Draw axes from center in screen-projected directions
  QPointF center(cx, cy);

  if (xScreenLen > 0.01f)
    drawLineScreen(painter, center,
                   center + QPointF(xDir.x(), xDir.y()) / xScreenLen * drawLen,
                   xPen);
  if (yScreenLen > 0.01f)
    drawLineScreen(painter, center,
                   center + QPointF(yDir.x(), yDir.y()) / yScreenLen * drawLen,
                   yPen);
  if (zScreenLen > 0.01f)
    drawLineScreen(painter, center,
                   center + QPointF(zDir.x(), zDir.y()) / zScreenLen * drawLen,
                   zPen);

  // Labels at tip of each axis
  painter.setFont(QFont("Consolas", 9, QFont::Bold));
  if (xScreenLen > 0.01f) {
    QPointF tip =
        center + QPointF(xDir.x(), xDir.y()) / xScreenLen * (drawLen + 10);
    painter.setPen(QColor(255, 80, 80));
    painter.drawText(tip.x() - 5, tip.y() + 4, QStringLiteral("X"));
  }
  if (yScreenLen > 0.01f) {
    QPointF tip =
        center + QPointF(yDir.x(), yDir.y()) / yScreenLen * (drawLen + 10);
    painter.setPen(QColor(80, 255, 80));
    painter.drawText(tip.x() - 5, tip.y() + 4, QStringLiteral("Y"));
  }
  if (zScreenLen > 0.01f) {
    QPointF tip =
        center + QPointF(zDir.x(), zDir.y()) / zScreenLen * (drawLen + 10);
    painter.setPen(QColor(80, 130, 255));
    painter.drawText(tip.x() - 5, tip.y() + 4, QStringLiteral("Z"));
  }
}

void Telemetry3DWidget::updateOrientation(const DataPacket &packet) {
  m_roll = packet.angx;
  m_pitch = packet.angy;
  m_yaw = packet.angz;

  //    m_position.setX(packet.sx);
  //    m_position.setY(packet.sy);
  //    m_position.setZ(packet.sz);
  // Commented cause NOW we don't need it )))
  //    m_velocity.setX(packet.vx);
  //    m_velocity.setY(packet.vy);
  //    m_velocity.setZ(packet.vz);

  //    m_trail.append(m_position);
  //    if (m_trail.size() > MAX_TRAIL_POINTS) {
  //        m_trail.removeFirst();
  //    }
  // below is commented because of using QTimer for it )))
  // update();
}

void Telemetry3DWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = true;
    m_lastMousePos = event->pos();
    setCursor(Qt::ClosedHandCursor);
  }
}

void Telemetry3DWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_isDragging) {
    QPoint delta = event->pos() - m_lastMousePos;
    // Horizontal mouse → orbit azimuth (yaw)
    m_cameraAngleY -= delta.x() * 0.5f;
    // Vertical mouse → change elevation (pitch)
    m_cameraAngleX += delta.y() * 0.5f;
    m_cameraAngleX = qBound(-89.0f, m_cameraAngleX, 89.0f);

    m_lastMousePos = event->pos();
    update();
  }
}

void Telemetry3DWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = false;
    unsetCursor();
  }
}

void Telemetry3DWidget::wheelEvent(QWheelEvent *event) {
  float delta = -event->angleDelta().y() * 0.01f;
  m_cameraDistance += delta;
  m_cameraDistance = qBound(2.0f, m_cameraDistance, 50.0f);
  update();
}
