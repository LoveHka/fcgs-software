#include "main_window.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	
	auto* central = new QWidget(this);
	auto* layout = new QVBoxLayout(central);

	valueLabel_ = new QLabel("0.0", this);

	valueLabel_->setAlignment(Qt::AlignCenter);
	valueLabel_->setStyleSheet("font-size: 24px;");

	layout->addWidget(valueLabel_);
	setCentralWidget(central);

	timer_ = new QTimer(this);
	connect(timer_, &QTimer::timeout, this, &MainWindow::onUpdate);

	timer_->start(50);
}

void MainWindow::onUpdate()
{
	static double t = 0.0;
	t += 0.05;
	
	double value = std::sin(t);

	valueLabel_->setText(QString::number(value, 'f', 3));
}
