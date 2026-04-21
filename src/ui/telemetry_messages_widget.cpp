// ui/telemetry_messages_widget.cpp
#include "telemetry_messages_widget.h"
#include <QDateTime>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

TelemetryMessagesWidget::TelemetryMessagesWidget(QWidget *parent)
    : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);

  m_log = new QPlainTextEdit(this);
  m_log->setReadOnly(true);
  m_log->setFont(QFont("Consolas", 9));
  m_log->setStyleSheet(
      "background: #1a1a2e; color: #1fbe8f; border: 1px solid #404060;");

  auto *clearBtn = new QPushButton("Очистить лог", this);
  connect(clearBtn, &QPushButton::clicked, m_log, &QPlainTextEdit::clear);

  layout->addWidget(m_log, 1);
  layout->addWidget(clearBtn);
}

void TelemetryMessagesWidget::appendMessage(const QString &msg) {
  QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_log->appendPlainText(QString("[%1] %2").arg(ts, msg));
  // Автопрокрутка вниз
  m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}
