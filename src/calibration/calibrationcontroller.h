// calibration/calibrationcontroller.h
#pragma once

#include "calibration/magcalibrator.h"
#include "datasource/serialreader.h"
#include <QObject>
#include <QString>
#include <QVector3D>

class CalibrationController : public QObject {
  Q_OBJECT
public:
  explicit CalibrationController(SerialReader *reader,
                                 QObject *parent = nullptr);

  bool isCollecting() const { return m_collecting; }

public slots:
  void startMagCalibration();
  void stopCollection();
  void compute();
  void apply();

signals:
  void statusChanged(const QString &text);
  void samplesChanged(int count);
  void calibrationReady();

private slots:
  void onPacket(const DataPacket &p);

private:
  SerialReader *m_reader{nullptr};
  MagCalibrator m_calibrator;
  bool m_collecting{false};
};
