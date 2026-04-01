#include "main_window.h"

#include "datasource/serialreader.h"
#include "datasource/telemetry_protocol.h"
#include "ui/telemetry_chart_widget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>

namespace
{
QFrame* createPanel(QWidget* parent = nullptr)
{
    auto* frame = new QFrame(parent);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return frame;
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("FCGS Telemetry");

    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    auto* leftPanel = createPanel(central);
    auto* middlePanel = createPanel(central);
    auto* rightPanel = createPanel(central);

    rootLayout->addWidget(leftPanel, 1);
    rootLayout->addWidget(middlePanel, 1);
    rootLayout->addWidget(rightPanel, 1);

    setCentralWidget(central);

    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(6, 6, 6, 6);
    leftLayout->setSpacing(6);

    m_charts[0] = new TelemetryChartWidget("Acceleration X", -20.0, 20.0, 10.0, leftPanel);
    m_charts[1] = new TelemetryChartWidget("Acceleration Y", -20.0, 20.0, 10.0, leftPanel);
    m_charts[2] = new TelemetryChartWidget("Acceleration Z", -20.0, 20.0, 10.0, leftPanel);

    for (auto* chart : m_charts) {
        leftLayout->addWidget(chart, 1);
    }

    reader = new SerialReader(this);
    connect(reader, &SerialReader::packetReady, this, &MainWindow::onPacket);

    const auto ports = getArduinoPorts();

    if (ports.isEmpty()) {
        QMessageBox::critical(this, "Error", "No Arduino-like ports found");
        return;
    }

    QStringList portNames;
    portNames.reserve(ports.size());

    for (const auto& p : ports) {
        portNames << p.portName() + " (" + p.description() + ")";
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this,
        "Select Arduino Port",
        "Available ports:",
        portNames,
        0,
        false,
        &ok
    );

    if (!ok) {
        return;
    }

    const int index = portNames.indexOf(selected);
    if (index >= 0) {
        reader->open(ports[index].portName());
    }
}

void MainWindow::onPacket(const DataPacket& p)
{
    const double timeSec = static_cast<double>(p.time) / 1000.0;

    m_charts[0]->appendPoint(timeSec, p.sx);
    m_charts[1]->appendPoint(timeSec, p.sy);
    m_charts[2]->appendPoint(timeSec, p.sz);
}