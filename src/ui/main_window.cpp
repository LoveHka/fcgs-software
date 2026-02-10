#include "main_window.h"
#include "datasource/serialreader.h"
#include "datasource/datapacket.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    series = new QLineSeries(this);
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Real-time Data");

    axisX = new QValueAxis(chart);
    axisX->setRange(0, 10);
    axisX->setTitleText("Time");
    axisX->setLabelFormat("%.2f");

    auto *axisY = new QValueAxis(chart);
    axisY->setRange(0, 100);
    axisY->setTitleText("Value");
    axisY->setLabelFormat("%.2f");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(chartView);
    
    reader = new SerialReader(this);

    connect(reader, &SerialReader::packetReady, this, &MainWindow::onPacket);

    reader->open("/dev/ttyUSB0");
    series->append(0, 0);
    series->append(2, 10);
    series->append(3, 100);
    series->append(4, 200);
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


