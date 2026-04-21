// ui/magcalibrationwidget.h
#pragma once

#include "calibration/calibrationcontroller.h"
#include <QWidget>
#include <qdialog.h>
#include <qtmetamacros.h>
#include <qwidget.h>

class QLabel;
class QPushButton;
class QTextEdit;
class CalibrationController;

class MagCalibrationWidget : public QWidget {
  Q_OBJECT
public:
  explicit MagCalibrationWidget(CalibrationController *controller,
                                QWidget *parent = nullptr);

private slots:
  void onStatusChanged(const QString &text);
  void onSamplesChanged(int count);
  void onCalibrationReady();

private:
  CalibrationController *m_controller{nullptr};

  QLabel *m_samplesLabel{nullptr};
  QLabel *m_statusLabel{nullptr};
  QTextEdit *m_resultView{nullptr};
};
