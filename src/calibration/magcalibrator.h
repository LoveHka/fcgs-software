// calibration/magcalibrator.h
#pragma once

#include <QVector3D>
#include <QVector>
#include <array>
// Класс для калибровки магнитометра (базовые функции)
class MagCalibrator {
public:
  void reset();
  void addSample(const QVector3D &sample);

  bool isEnough() const;
  void compute();

  int sampleCount() const { return m_points.size(); }

  QVector3D offset() const { return m_offset; }
  const std::array<float, 9> &matrix() const { return m_matrix; }

private:
  QVector<QVector3D> m_points;
  QVector3D m_offset{0.0f, 0.0f, 0.0f};
  std::array<float, 9> m_matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                0.0f, 0.0f, 0.0f, 1.0f};
};
