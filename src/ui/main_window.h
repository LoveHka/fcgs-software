#pragma once

#include <QMainWindow>

class QLabel;
class QTimer;

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);
	private slots:
		void onUpdate();
	private:
	QLabel* valueLabel_;
	QTimer* timer_;
};

