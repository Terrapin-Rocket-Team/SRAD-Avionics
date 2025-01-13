#include "MainWindow.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , rocketController(new RocketController(this))
{
    setupUi();
    
    // Connect signals/slots
    connect(rocketController, &RocketController::connectionStatusChanged,
            this, &MainWindow::updateConnectionStatus);
    connect(rocketController, &RocketController::batteryLevelChanged,
            this, &MainWindow::updateBatteryLevel);
    connect(rocketController, &RocketController::systemStatusUpdated,
            this, &MainWindow::updateSystemStatus);
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Status section
    QGroupBox *statusGroup = new QGroupBox("System Status", this);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    connectionStatus = new QLabel("Disconnected", statusGroup);
    batteryLevel = new QLabel("Battery: ---%", statusGroup);
    statusLayout->addWidget(connectionStatus);
    statusLayout->addWidget(batteryLevel);
    
    // Control buttons
    QGroupBox *controlGroup = new QGroupBox("System Controls", this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    QPushButton *testPayloadBtn = new QPushButton("Test Payload", controlGroup);
    QPushButton *testAvionicsBtn = new QPushButton("Test Avionics", controlGroup);
    QPushButton *testAirbrakesBtn = new QPushButton("Test Airbrakes", controlGroup);
    
    connect(testPayloadBtn, &QPushButton::clicked,
            rocketController, &RocketController::testPayload);
    connect(testAvionicsBtn, &QPushButton::clicked,
            rocketController, &RocketController::testAvionics);
    connect(testAirbrakesBtn, &QPushButton::clicked,
            rocketController, &RocketController::testAirbrakes);
    
    controlLayout->addWidget(testPayloadBtn);
    controlLayout->addWidget(testAvionicsBtn);
    controlLayout->addWidget(testAirbrakesBtn);
    
    mainLayout->addWidget(statusGroup);
    mainLayout->addWidget(controlGroup);
}