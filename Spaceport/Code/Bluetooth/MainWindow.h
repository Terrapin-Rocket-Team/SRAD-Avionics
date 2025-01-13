#pragma once

#include <QMainWindow>
#include <QPushButton>
#include "RocketController.h"

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void updateConnectionStatus(bool connected) {}
    void updateBatteryLevel(float level) {}
    void updateSystemStatus(const QString& system, const QString& status) {}

    RocketController *rocketController;
    QLabel *connectionStatus;
    QLabel *batteryLevel;
}; 