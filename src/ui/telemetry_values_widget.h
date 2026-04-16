// ui/telemetry_values_widget.h
#pragma once

#include <QWidget>
#include <QMap>
#include <QString>
#include "datasource/telemetry_protocol.h"

class TelemetryValuesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TelemetryValuesWidget(QWidget* parent = nullptr);

public slots:
    void updateValues(const DataPacket& packet);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawValueRow(QPainter& painter, int row, const QString& label, const QString& value, const QString& unit);
    void drawSeparator(QPainter& painter, int y);
    void drawHeader(QPainter& painter);
    void drawScrollbar(QPainter& painter, int maxScroll);

    struct ValueDisplay {
        QString label;
        QString value;
        QString unit;
    };

    QVector<ValueDisplay> m_values;
    int m_rowHeight{28};
    int m_headerHeight{30};
    int m_margin{10};
    QColor m_labelColor{0x88, 0x88, 0xaa};
    QColor m_valueColor{0xff, 0xff, 0xff};
    QColor m_unitColor{0x66, 0x66, 0x88};
    QColor m_bgColor{0x1a, 0x1a, 0x2e};
    QColor m_altRowColor{0x22, 0x22, 0x3a};

    int m_scrollOffset{0};
    int m_visibleRows{0};

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_isDragging{false};
    QPoint m_lastMousePos;
};
