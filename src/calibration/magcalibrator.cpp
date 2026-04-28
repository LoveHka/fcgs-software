// calibration/magcalibrator.cpp

#include "calibration/magcalibrator.h"

#include <Eigen/Dense>
#include <QVector3D>
#include <QVector>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

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

  int N = m_points.size();

  // Матрица дизайна D (Nx9) и вектор единиц O (Nx1)
  // Мы решаем систему D * v = O, где v - коэффициенты уравнения эллипсоида
  Eigen::MatrixXd D(N, 9);
  Eigen::VectorXd O = Eigen::VectorXd::Ones(N);

  for (int i = 0; i < N; ++i) {
    double x = (m_points)[i].x();
    double y = (m_points)[i].y();
    double z = (m_points)[i].z();

    D(i, 0) = x * x;
    D(i, 1) = y * y;
    D(i, 2) = z * z;
    D(i, 3) = 2.0 * x * y;
    D(i, 4) = 2.0 * x * z;
    D(i, 5) = 2.0 * y * z;
    D(i, 6) = 2.0 * x;
    D(i, 7) = 2.0 * y;
    D(i, 8) = 2.0 * z;
  }

  // 2. Решаем переопределенную систему уравнений методом наименьших квадратов
  // через SVD-разложение (самый точный и стабильный метод)
  Eigen::VectorXd v =
      D.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(O);

  // 3. Формируем квадратичную матрицу эллипсоида A и вектор b
  Eigen::Matrix3d A;
  A << v(0), v(3), v(4), v(3), v(1), v(5), v(4), v(5), v(2);

  Eigen::Vector3d b(v(6), v(7), v(8));

  // 4. Вычисляем смещения (Hard-Iron, центр эллипсоида)
  // Уравнение центра: c = -A^-1 * b
  Eigen::Vector3d center = -A.inverse() * b;
  m_offset.setX(static_cast<float>(center(0)));
  m_offset.setY(static_cast<float>(center(1)));
  m_offset.setZ(static_cast<float>(center(2)));

  // 5. Вычисляем матрицу калибровки (Soft-Iron, условие сферичности)
  // Чтобы получить сферу, нам нужно найти матрицу W, такую что W^T * W = A / k,
  // где k - масштабирующий фактор эллипсоида.
  double k = center.transpose() * A * center + 1.0;
  Eigen::Matrix3d A_scaled = A / k;

  // Извлекаем квадратный корень из симметричной матрицы через её собственные
  // значения
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(A_scaled);

  // Берем корни собственных значений. cwiseMax(0.0) страхует от погрешностей
  // вычислений (если матрица вдруг перестала быть положительно определенной)
  Eigen::Matrix3d D_sqrt =
      es.eigenvalues().cwiseMax(0.0).cwiseSqrt().asDiagonal();

  // Собираем обратно матрицу преобразования (Soft-Iron)
  Eigen::Matrix3d W =
      es.eigenvectors() * D_sqrt * es.eigenvectors().transpose();

  // 6. Записываем результат в переданный std::array (построчно)
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      m_matrix[i * 3 + j] = static_cast<float>(W(i, j));
    }
  }
}
