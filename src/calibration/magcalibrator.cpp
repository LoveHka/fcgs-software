// calibration/magcalibrator.cpp

#include "calibration/magcalibrator.h"

#include <algorithm>

void MagCalibrator::reset() {
  m_points.clear();
  m_offset = QVector3D(0.0f, 0.0f, 0.0f);
  m_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

void MagCalibrator::addSample(const QVector3D &sample) {
  m_points.append(sample);
}

bool MagCalibrator::isEnough() const { return m_points.size() >= 500; }

void MagCalibrator::compute() {
  if (m_points.isEmpty())
    return;

  QVector3D minV(1e9f, 1e9f, 1e9f);
  QVector3D maxV(-1e9f, -1e9f, -1e9f);

  for (const auto &p : m_points) {
    minV.setX(std::min(minV.x(), p.x()));
    minV.setY(std::min(minV.y(), p.y()));
    minV.setZ(std::min(minV.z(), p.z()));

    maxV.setX(std::max(maxV.x(), p.x()));
    maxV.setY(std::max(maxV.y(), p.y()));
    maxV.setZ(std::max(maxV.z(), p.z()));
  }

  m_offset = (minV + maxV) * 0.5f;

  const QVector3D halfRange = (maxV - minV) * 0.5f;
  const float hx = std::max(halfRange.x(), 1e-6f);
  const float hy = std::max(halfRange.y(), 1e-6f);
  const float hz = std::max(halfRange.z(), 1e-6f);

  const float avg = (hx + hy + hz) / 3.0f;

  // Простая 3x3 матрица масштабирования по осям.
  // Это не полный эллипсоид-фит, но уже рабочая калибровка на старте.
  m_matrix = {avg / hx, 0.0f, 0.0f, 0.0f, avg / hy, 0.0f, 0.0f, 0.0f, avg / hz};
}
