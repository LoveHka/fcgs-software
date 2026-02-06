#pragma once

#include <QMainWindow>
#include <memory>

class QChartView;
class QLineSeries;
class QTimer;

class DataSource;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr); //init
    ~MainWindow(); //destructor

private slots:
	void onUpdate(); 

private:
    std::unique_ptr<DataSource> dataSource;
    
    QChartView* chartView = nullptr;
    QLineSeries* series = nullptr;
    QTimer* timer = nullptr;

    double time = 0.0;
};



