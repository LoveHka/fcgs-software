#pragma once

#include <QMainWindow>
#include <memory>

class QChartView;
class QLineSeries;
class QValueAxis;

class SerialReader;
struct DataPacket;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr); //init

private slots:
	void onPacket(const DataPacket& p); 

private:
    
    
    QChartView* chartView{nullptr};
    QLineSeries* series{nullptr};
    QValueAxis* axisX{nullptr};

    SerialReader* reader{nullptr};

    
};



