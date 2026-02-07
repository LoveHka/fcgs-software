#include "main_window.h"
#include "datasource/datasource.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QTimer>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    dataSource(std::make_unique<DataSource>()), time(0.0) 
{
    series = new QLineSeries(this);
    
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Real-time Data");

    auto *axisX = new QValueAxis(chart);
    axisX->setRange(0, 10);
    axisX->setTitleText("Time");
    axisX->setLabelFormat("%.2f");

    auto *axisY = new QValueAxis(chart);
    axisY->setRange(0, 10);
    axisY->setTitleText("Value");
    axisY->setLabelFormat("%.2f");

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
	
    if (series->count() > 1000) {
	    series->removePoints(0, series->count() - 1000);
    }
    if (time > 10.0) {
    	auto axes = chartView->chart()->axes(Qt::Horizontal);
	if (!axes.isEmpty()) {
		if (auto *axisX = qobject_cast<QValueAxis*>(axes.first())) {
			axisX->setRange(time-10.0, time);
		}
    }

}
}

MainWindow::~MainWindow()
{
	if (timer && timer->isActive()) {
		timer->stop();
	}
}

