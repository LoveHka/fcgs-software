// ui/telemetry_values_widget.cpp

#include "ui/telemetry_values_widget.h"

#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QtMath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>

TelemetryValuesWidget::TelemetryValuesWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(250, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TelemetryValuesWidget::updateValues(const DataPacket& packet)
{
    m_values.clear();
    
    m_values.append({QStringLiteral("Время"), QString::number(packet.time, 'f', 2), QStringLiteral("мс")});
    m_values.append({QStringLiteral("Ускорение X"), QString::number(packet.ax, 'f', 3), QStringLiteral("g")});
    m_values.append({QStringLiteral("Ускорение Y"), QString::number(packet.ay, 'f', 3), QStringLiteral("g")});
    m_values.append({QStringLiteral("Ускорение Z"), QString::number(packet.az, 'f', 3), QStringLiteral("g")});
    m_values.append({QStringLiteral("Скорость X"), QString::number(packet.vx, 'f', 3), QStringLiteral("м/с")});
    m_values.append({QStringLiteral("Скорость Y"), QString::number(packet.vy, 'f', 3), QStringLiteral("м/с")});
    m_values.append({QStringLiteral("Скорость Z"), QString::number(packet.vz, 'f', 3), QStringLiteral("м/с")});
    m_values.append({QStringLiteral("Позиция X"), QString::number(packet.sx, 'f', 3), QStringLiteral("м")});
    m_values.append({QStringLiteral("Позиция Y"), QString::number(packet.sy, 'f', 3), QStringLiteral("м")});
    m_values.append({QStringLiteral("Позиция Z"), QString::number(packet.sz, 'f', 3), QStringLiteral("м")});
    m_values.append({QStringLiteral("Магнитометр X"), QString::number(packet.mx, 'f', 2), QStringLiteral("мкТл")});
    m_values.append({QStringLiteral("Магнитометр Y"), QString::number(packet.my, 'f', 2), QStringLiteral("мкТл")});
    m_values.append({QStringLiteral("Магнитометр Z"), QString::number(packet.mz, 'f', 2), QStringLiteral("мкТл")});
    m_values.append({QStringLiteral("Гироскоп X"), QString::number(packet.gx, 'f', 3), QStringLiteral("°/с")});
    m_values.append({QStringLiteral("Гироскоп Y"), QString::number(packet.gy, 'f', 3), QStringLiteral("°/с")});
    m_values.append({QStringLiteral("Гироскоп Z"), QString::number(packet.gz, 'f', 3), QStringLiteral("°/с")});
    m_values.append({QStringLiteral("Угол крена"), QString::number(packet.angx, 'f', 2), QStringLiteral("°")});
    m_values.append({QStringLiteral("Угол тангажа"), QString::number(packet.angy, 'f', 2), QStringLiteral("°")});
    m_values.append({QStringLiteral("Угол рыскания"), QString::number(packet.angz, 'f', 2), QStringLiteral("°")});
    m_values.append({QStringLiteral("Направление"), QString::number(packet.H, 'f', 2), QStringLiteral("°")});
    m_values.append({QStringLiteral("Давление"), QString::number(packet.P, 'f', 2), QStringLiteral("мм рт.ст.")});
    m_values.append({QStringLiteral("Температура"), QString::number(packet.T, 'f', 2), QStringLiteral("°C")});
    m_values.append({QStringLiteral("Баро-высота"), QString::number(packet.BarAlt, 'f', 2), QStringLiteral("м")});
    m_values.append({QStringLiteral("Скорость (модуль)"), QString::number(packet.V, 'f', 2), QStringLiteral("м/с")});
    m_values.append({QStringLiteral("Дистанция"), QString::number(packet.d, 'f', 2), QStringLiteral("м")});
    m_values.append({QStringLiteral("Цикл обработки"), QString::number(packet.Tcycle, 'f', 2), QStringLiteral("мс")});

    update();
}

void TelemetryValuesWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Background
    painter.fillRect(rect(), m_bgColor);

    // Border
    painter.setPen(QPen(QColor(0x40, 0x40, 0x60), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    // Calculate visible rows based on available height
    int availableHeight = height() - m_headerHeight - m_margin;
    m_visibleRows = std::max(1, availableHeight / m_rowHeight);

    // Clamp scroll offset
    int maxScroll = std::max(0, (int)m_values.size() - m_visibleRows);
    m_scrollOffset = qBound(0, m_scrollOffset, maxScroll);

    // Draw header
    drawHeader(painter);

    // Draw hint for scroll
    if ((int)m_values.size() > m_visibleRows) {
        painter.setFont(QFont("Arial", 8));
        painter.setPen(QColor(0x66, 0x66, 0x88));
        QString hint = QStringLiteral("Колёсико / Shift+ЛКМ для прокрутки");
        painter.drawText(m_margin, m_headerHeight + m_margin - 5, hint);
    }

    // Draw value rows
    for (int i = 0; i < m_visibleRows && (i + m_scrollOffset) < (int)m_values.size(); ++i) {
        int idx = i + m_scrollOffset;
        int y = m_headerHeight + m_margin + i * m_rowHeight;
        
        // Alternating row background
        if (i % 2 == 1) {
            painter.fillRect(m_margin, y, width() - 2 * m_margin, m_rowHeight, m_altRowColor);
        }

        drawValueRow(painter, i, m_values[idx].label, m_values[idx].value, m_values[idx].unit);
    }

    // Draw scrollbar
    if ((int)m_values.size() > m_visibleRows) {
        drawScrollbar(painter, maxScroll);
    }
}

void TelemetryValuesWidget::drawHeader(QPainter& painter)
{
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.setPen(QColor(0xff, 0xff, 0xff));
    
    QRect headerRect(m_margin, 0, width() - 2 * m_margin, m_headerHeight);
    painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("ТЕЛЕМЕТРИЯ"));

    // Separator line
    painter.setPen(QPen(QColor(0x40, 0x40, 0x60), 1));
    painter.drawLine(m_margin, m_headerHeight, width() - m_margin, m_headerHeight);
}

void TelemetryValuesWidget::drawValueRow(QPainter& painter, int row, const QString& label, const QString& value, const QString& unit)
{
    int y = m_headerHeight + m_margin + row * m_rowHeight;
    int labelWidth = width() / 3;
    int valueWidth = width() / 2;
    int unitWidth = width() / 6;

    // Label
    painter.setFont(QFont("Arial", 9));
    painter.setPen(m_labelColor);
    QRect labelRect(m_margin, y, labelWidth, m_rowHeight);
    painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);

    // Value
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.setPen(m_valueColor);
    QRect valueRect(labelWidth + m_margin, y, valueWidth, m_rowHeight);
    painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter, value);

    // Unit
    painter.setFont(QFont("Arial", 9));
    painter.setPen(m_unitColor);
    QRect unitRect(labelWidth + valueWidth + m_margin, y, unitWidth, m_rowHeight);
    painter.drawText(unitRect, Qt::AlignLeft | Qt::AlignVCenter, unit);

    // Separator line
    drawSeparator(painter, y + m_rowHeight);
}

void TelemetryValuesWidget::drawSeparator(QPainter& painter, int y)
{
    painter.setPen(QPen(QColor(0x30, 0x30, 0x50), 1));
    painter.drawLine(m_margin, y, width() - m_margin, y);
}

void TelemetryValuesWidget::drawScrollbar(QPainter& painter, int maxScroll)
{
    int scrollbarWidth = 6;
    int scrollbarX = width() - scrollbarWidth - 2;
    int scrollbarAreaTop = m_headerHeight + m_margin;
    int scrollbarAreaHeight = height() - scrollbarAreaTop - m_margin;
    
    // Background track
    painter.fillRect(scrollbarX, scrollbarAreaTop, scrollbarWidth, scrollbarAreaHeight, QColor(0x20, 0x20, 0x30));
    
    // Thumb
    float thumbRatio = (float)m_visibleRows / m_values.size();
    int thumbHeight = std::max(20, (int)(scrollbarAreaHeight * thumbRatio));
    float scrollRatio = (maxScroll > 0) ? (float)m_scrollOffset / maxScroll : 0;
    int thumbY = scrollbarAreaTop + (int)(scrollRatio * (scrollbarAreaHeight - thumbHeight));
    
    painter.fillRect(scrollbarX, thumbY, scrollbarWidth, thumbHeight, QColor(0x60, 0x60, 0x80));
}

void TelemetryValuesWidget::wheelEvent(QWheelEvent* event)
{
    int delta = -event->angleDelta().y() / 120;
    m_scrollOffset += delta * 3;
    m_scrollOffset = qBound(0, m_scrollOffset, std::max(0, (int)m_values.size() - m_visibleRows));
    update();
}

void TelemetryValuesWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void TelemetryValuesWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging) {
        int delta = event->pos().y() - m_lastMousePos.y();
        m_scrollOffset -= delta / m_rowHeight;
        m_scrollOffset = qBound(0, m_scrollOffset, std::max(0, (int)m_values.size() - m_visibleRows));
        m_lastMousePos = event->pos();
        update();
    }
}

void TelemetryValuesWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        m_isDragging = false;
        unsetCursor();
    }
}
