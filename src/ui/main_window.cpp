#include "main_window.h"
#include "datasource/datasource.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QTimer>




MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    dataSource(std::make_unique<DataSource>()) 
{
    series = new QLineSeries();
    
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();

    auto *axisX = new QValueAxis;
    axisX->setRange(0, 10);
    axisX->setTitleText("Time");

    auto *axisY = new QValueAxis;
    axisY->setRange(0, 10);
    axisY->setTitleText("Value");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(chartView);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onUpdate);
    timer->start(16);
}

void MainWindow::onUpdate()
{
	time += 0.016;

    double y = dataSource->next();
    series->append(time, y);

    if (time > 10.0) {
        series->remove(0);
        chartView->chart()->axisX()->setRange(time - 10.0, time);
    }
}
