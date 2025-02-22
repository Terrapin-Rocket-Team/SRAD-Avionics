//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothClient.h"

BluetoothClient::BluetoothClient(Stream &outSerial) : outSerial(outSerial) {}

BluetoothClient::~BluetoothClient() {
    BLEDevice::deinit();
}

bool BluetoothClient::start(const std::string &serverName) {
    BLEDevice::init(serverName + "-client");

    BLEScan *pBLEScan = BLEDevice::getScan();
    AdvertisingScanHandler handler = AdvertisingScanHandler(*this);
    pBLEScan->setAdvertisedDeviceCallbacks(&handler);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(60); // scan for 60sec
    initialized = true;
}

void BluetoothClient::update(Stream& inputSerial) {
    if (initialized && connected) {
        if (!inputSerial.available()) return;

        uint16_t size = 0;
        inputSerial.readBytes(reinterpret_cast<char *>(&size), sizeof(uint16_t));

        if (size != 0 && size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            uint8_t buffer[size];
            inputSerial.readBytes(buffer, size);
            send(buffer, size);
        }
    }
}

bool BluetoothClient::send(uint8_t *data, uint16_t length) {
    if (remoteRx->canWrite()) {
        remoteRx->writeValue(data, length);
        return true;
    }
    return false;
}

uint16_t BluetoothClient::read(uint8_t *data) {
    if (remoteRx->canRead()) {
        uint8_t* raw = remoteRx->readRawData();

        uint16_t size = *reinterpret_cast<uint16_t*>(raw);
        *data = reinterpret_cast<uint8_t>(raw + sizeof(uint16_t));
        return size;
    }

    return 0;
}

bool BluetoothClient::isInitialized() {
    return initialized;
}

bool BluetoothClient::isConnected() {
    return connected;
}

void BluetoothClient::handleDeviceCallback(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == serverName) { // found the server
        advertisedDevice.getScan()->stop();
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());

        pClient = BLEDevice::createClient();
        pClient->connect(*pServerAddress);

        pRemoteService = pClient->getService(serverName + "-service");
        if (pRemoteService == nullptr) {
            //fail
            connected = false;
            return;
        }

        remoteRx = pRemoteService->getCharacteristic(serverName + "-charRX"); //server's rx
        remoteTx = pRemoteService->getCharacteristic(serverName + "-charTX"); //server's tx

        if (remoteRx == nullptr || remoteTx == nullptr) {
            connected = false;
            return;
        }

        remoteTx->registerForNotify([this] (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
            handleTxCallback(pBLERemoteCharacteristic, pData, length, isNotify);
        });

        connected = true;
    }
}

void BluetoothClient::handleTxCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
    bool isNotify) {
    if (pBLERemoteCharacteristic == remoteTx) {
        const uint16_t size = *reinterpret_cast<uint16_t *>(pData);
        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            outSerial.write(pData + sizeof(uint16_t), size);
        } else {
            //this is bad
        }
    }
}

AdvertisingScanHandler::AdvertisingScanHandler(BluetoothClient &client) : bluetoothClient(client) {}

void AdvertisingScanHandler::onResult(BLEAdvertisedDevice advertisedDevice) {
    bluetoothClient.handleDeviceCallback(advertisedDevice);
}
