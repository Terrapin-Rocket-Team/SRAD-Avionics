#include "RocketController.h"
#include <QDebug>

const QString RocketController::ROCKET_DEVICE_NAME = "ModelRocket";

RocketController::RocketController(QObject *parent) 
    : QObject(parent)
    , discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , socket(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this))
    , connectionTimer(new QTimer(this))
{
    // Setup Bluetooth discovery
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &RocketController::handleDeviceDiscovered);
    
    // Setup socket handling
    connect(socket, &QBluetoothSocket::connected, [this]() {
        emit connectionStatusChanged(true);
    });
    
    connect(socket, &QBluetoothSocket::disconnected, [this]() {
        emit connectionStatusChanged(false);
        connectionTimer->start(RECONNECT_INTERVAL_MS);
    });
    
    connect(socket, &QBluetoothSocket::readyRead,
            this, &RocketController::handleDataReceived);
    
    connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
            this, &RocketController::handleSocketError);
            
    // Setup reconnection timer
    connect(connectionTimer, &QTimer::timeout, this, &RocketController::checkConnection);
}

bool RocketController::initializeSystems() {
    if (!socket->isOpen()) {
        return connectToRocket();
    }
    
    // Send initialization command
    sendCommand("INIT");
    return true;
}

void RocketController::testPayload() {
    if (!socket->isOpen()) {
        emit errorOccurred("Not connected to rocket");
        return;
    }
    sendCommand("TEST_PAYLOAD");
}

void RocketController::testAvionics() {
    if (!socket->isOpen()) {
        emit errorOccurred("Not connected to rocket");
        return;
    }
    sendCommand("TEST_AVIONICS");
}

void RocketController::testAirbrakes() {
    if (!socket->isOpen()) {
        emit errorOccurred("Not connected to rocket");
        return;
    }
    sendCommand("TEST_AIRBRAKES");
}

float RocketController::getBatteryLevel() {
    if (!socket->isOpen()) {
        emit errorOccurred("Not connected to rocket");
        return -1.0f;
    }
    sendCommand("GET_BATTERY");
    // Actual implementation would wait for response
    return 0.0f;
}

void RocketController::handleDeviceDiscovered(const QBluetoothDeviceInfo &device) {
    if (device.name() == ROCKET_DEVICE_NAME) {
        discoveryAgent->stop();
        socket->connectToDevice(device.address(), QBluetoothUuid::SerialPort);
    }
}

void RocketController::handleDataReceived() {
    const QByteArray data = socket->readAll();
    processSystemStatus(data);
}

void RocketController::processSystemStatus(const QByteArray& data) {
    // Example protocol implementation
    // Format: SYSTEM:STATUS:VALUE
    QString message = QString::fromUtf8(data);
    QStringList parts = message.split(':');
    
    if (parts.size() >= 3) {
        QString system = parts[0];
        QString status = parts[1];
        QString value = parts[2];
        
        emit systemStatusUpdated(system, status);
        
        if (system == "BATTERY") {
            bool ok;
            float level = value.toFloat(&ok);
            if (ok) {
                emit batteryLevelChanged(level);
            }
        }
    }
}
