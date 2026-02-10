#include <QApplication>

#include <QLoggingCategory>
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("*.debug=true\nqt.*.debug=false");
    QApplication app(argc, argv);

	MainWindow w;
	w.resize(800, 600);
	w.show();

	return app.exec();
}
