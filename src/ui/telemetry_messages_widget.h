// ui/Telemetry_Messages_Widget.h
#pragma once
#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

class TelemetryMessagesWidget : public QWidget {
  Q_OBJECT
public:
  explicit TelemetryMessagesWidget(QWidget *parent = nullptr);

public slots:
  void appendMessage(const QString &msg);

private:
  QPlainTextEdit *m_log;
};
