#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QTimer>

class RocketController : public QObject {
    Q_OBJECT

public:
    explicit RocketController(QObject *parent = nullptr);
    ~RocketController();

    // System control methods
    bool initializeSystems();
    void testPayload();
    void testAvionics();
    void testAirbrakes();
    float getBatteryLevel();
    QImage getCameraPreview();

signals:
    void connectionStatusChanged(bool connected);
    void batteryLevelChanged(float level);
    void systemStatusUpdated(const QString& system, const QString& status);
    void errorOccurred(const QString& error);

private slots:
    void handleDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void handleSocketError(QBluetoothSocket::SocketError error);
    void handleDataReceived();
    void checkConnection();

private:
    static const QString ROCKET_DEVICE_NAME;
    static const int RECONNECT_INTERVAL_MS = 5000;
    
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothSocket *socket;
    QTimer *connectionTimer;
    
    bool connectToRocket();
    void sendCommand(const QByteArray& cmd);
    void processSystemStatus(const QByteArray& data);
};
