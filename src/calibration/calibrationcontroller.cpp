// calibration/calibrationcontroller.cpp
#include "calibration/calibrationcontroller.h"
#include <qobject.h>

CalibrationController::CalibrationController(SerialReader *reader,
                                             QObject *parent)
    : QObject(parent), m_reader(reader) {
  connect(m_reader, &SerialReader::packetReady, this,
          &CalibrationController::onPacket);
}

void CalibrationController::startMagCalibration() {
  m_calibrator.reset();
  m_collecting = true;

  CommandPacket cmd{};
  cmd.cmd = CMD_SET_MODE;
  cmd.params[0] = static_cast<float>(MODE_MAG_CALIB);
  m_reader->sendCommand(cmd);

  emit statusChanged("MAG calibration started");
  emit samplesChanged(0);
}

void CalibrationController::stopCollection() {
  m_collecting = false;
  emit statusChanged("Collection stopped");
}

void CalibrationController::compute() {
  m_collecting = false;

  if (!m_calibrator.isEnough()) {
    emit statusChanged("Not enough samples for calibration");
    return;
  }

  m_calibrator.compute();
  emit calibrationReady();
  emit statusChanged("Calibration computed");
  auto &mat = m_calibrator.matrix();
  auto off = m_calibrator.offset();
  QString res = QString("Offset:\n %1  %2  %3 \nMatrix: \n")
                    .arg(off.x(), 8, 'f', 9)
                    .arg(off.y(), 8, 'f', 9)
                    .arg(off.z(), 8, 'f', 9);
  for (int r = 0; r < 3; r++) {
    res += QString("| %1  %2  %3 |\n")
               .arg(mat[r * 3], 8, 'f', 9)
               .arg(mat[r * 3 + 1], 8, 'f', 9)
               .arg(mat[r * 3 + 2], 8, 'f', 9);
  }
  emit statusChanged(res);
}

void CalibrationController::apply() {
  if (!m_calibrator.isEnough()) {
    emit statusChanged("Calibration was not computed");
    return;
  }

  CommandPacket cmd{};
  cmd.cmd = CMD_SET_MAG_CALIB;

  const QVector3D off = m_calibrator.offset();
  const auto &M = m_calibrator.matrix();

  cmd.params[0] = off.x();
  cmd.params[1] = off.y();
  cmd.params[2] = off.z();

  for (int i = 0; i < 9; ++i)
    cmd.params[3 + i] = M[i];

  m_reader->sendCommand(cmd);

  CommandPacket mode{};
  mode.cmd = CMD_SET_MODE;
  mode.params[0] = static_cast<float>(MODE_NORMAL);
  m_reader->sendCommand(mode);

  emit statusChanged("Calibration sent to device");
}

void CalibrationController::onPacket(const DataPacket &p) {
  if (!m_collecting)
    return;

  m_calibrator.addSample(QVector3D(p.mx, p.my, p.mz));
  emit samplesChanged(m_calibrator.sampleCount());
}
