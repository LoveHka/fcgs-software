// ui/main_window.cpp
#include "main_window.h"
#include "datasource/serialreader.h"
#include "datasource/datapacket.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{   
    // Серия точек для графика
    series = new QLineSeries(this);
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Real-time Data");
    // Создаем ось
    axisX = new QValueAxis(chart);
    axisX->setRange(0, 10);
    axisX->setTitleText("Time");
    axisX->setLabelFormat("%.2f");
    // Создаем ось
    auto *axisY = new QValueAxis(chart);
    axisY->setRange(-8, 8);
    axisY->setTitleText("Value");
    axisY->setLabelFormat("%.2f");
    // Добавляем оси
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(chartView);
    
    reader = new SerialReader(this);

    connect(reader, &SerialReader::packetReady, this, &MainWindow::onPacket);

    // --- ВЫБОР ПОРТА ---
    auto ports = getArduinoPorts();

    if (ports.isEmpty()) {
        QMessageBox::critical(this, "Error", "No Arduino-like ports found");
        return;
    }

    QStringList portNames;
    for (const auto& p : ports) {
        portNames << p.portName() + " (" + p.description() + ")";
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(
        this,
        "Select Arduino Port",
        "Available ports:",
        portNames,
        0,
        false,
        &ok
    );

    if (!ok) return;

    int index = portNames.indexOf(selected);
    if (index >= 0) {
        reader->open(ports[index].portName());
        qDebug() << "PORT: " << index << " --- " << ports[index].portName();
    }
}

void MainWindow::onPacket(const DataPacket& p)
{
    qDebug() << "Packet here: " << p.time;
    auto time = p.time / 1000;    // millis to seconds
    series->append(time, p.ax);

    if (series->count() > 1000) {
        series->removePoints(0, series->count() - 1000);
    }

    axisX->setRange(time - 10.0, time);
}


