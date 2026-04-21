// ui/magcalibrationwidget.cpp
#include "ui/magcalibrationwidget.h"

#include "calibration/calibrationcontroller.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

MagCalibrationWidget::MagCalibrationWidget(CalibrationController *controller,
                                           QWidget *parent)
    : QWidget(parent), m_controller(controller) {
  auto *root = new QVBoxLayout(this);

  auto *controls = new QHBoxLayout();

  auto *startBtn = new QPushButton("Start MAG calibration", this);
  auto *stopBtn = new QPushButton("Stop", this);
  auto *computeBtn = new QPushButton("Compute", this);
  auto *applyBtn = new QPushButton("Apply to device", this);

  controls->addWidget(startBtn);
  controls->addWidget(stopBtn);
  controls->addWidget(computeBtn);
  controls->addWidget(applyBtn);
  root->addLayout(controls);

  m_samplesLabel = new QLabel("Samples: 0", this);
  m_statusLabel = new QLabel("Idle", this);

  root->addWidget(m_samplesLabel);
  root->addWidget(m_statusLabel);

  m_resultView = new QTextEdit(this);
  m_resultView->setReadOnly(true);
  root->addWidget(m_resultView, 1);

  connect(startBtn, &QPushButton::clicked, m_controller,
          &CalibrationController::startMagCalibration);

  connect(stopBtn, &QPushButton::clicked, m_controller,
          &CalibrationController::stopCollection);

  connect(computeBtn, &QPushButton::clicked, m_controller,
          &CalibrationController::compute);

  connect(applyBtn, &QPushButton::clicked, m_controller,
          &CalibrationController::apply);

  connect(m_controller, &CalibrationController::statusChanged, this,
          &MagCalibrationWidget::onStatusChanged);

  connect(m_controller, &CalibrationController::samplesChanged, this,
          &MagCalibrationWidget::onSamplesChanged);

  connect(m_controller, &CalibrationController::calibrationReady, this,
          &MagCalibrationWidget::onCalibrationReady);
}

void MagCalibrationWidget::onStatusChanged(const QString &text) {
  m_statusLabel->setText("Status: " + text);
}

void MagCalibrationWidget::onSamplesChanged(int count) {
  m_samplesLabel->setText(QString("Samples: %1").arg(count));
}

void MagCalibrationWidget::onCalibrationReady() {
  m_resultView->append("Calibration computed.");
  m_resultView->append("Offset and matrix are ready to send.");
}
