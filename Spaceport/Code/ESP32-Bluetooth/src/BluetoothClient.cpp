//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothClient.h"

#include <HardwareSerial.h>

BluetoothClient::BluetoothClient(Stream &outSerial) : outSerial(outSerial) {}

BluetoothClient::~BluetoothClient() {
    BLEDevice::deinit();
}

bool BluetoothClient::start(const std::string &serverName) {
    this->serverName = serverName;
    BLEDevice::init("");

    Serial.println("BluetoothClient::start");
    pClient = BLEDevice::createClient();

    BLEScan *pBLEScan = BLEDevice::getScan();
    pScanHandler = new AdvertisingScanHandler(this);
    pBLEScan->setAdvertisedDeviceCallbacks(pScanHandler);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(30);
    pBLEScan->setInterval(29);

    Serial.println("STARTED SCANNING...");
    pBLEScan->start(100);
    initialized = true;
    return initialized;
}

void BluetoothClient::update(Stream& inputSerial) {
    if (!connected && pServerAddress != nullptr) {
        pClient->connect(*pServerAddress);

        Serial.println("Connected to server!");

        pRemoteService = pClient->getService("cba1d466-344c-4be3-ab3f-189f80dd7518");
        if (pRemoteService == nullptr) {
            //fail
            Serial.println("FAILED TO GET SERVICE");
            connected = false;
            return;
        }

        remoteRx = pRemoteService->getCharacteristic("9c15bf50-4a60-40"); //server's rx
        remoteTx = pRemoteService->getCharacteristic("5159c5ac-b886-42"); //server's tx

        if (remoteRx == nullptr || remoteTx == nullptr) {
            Serial.println("FAILED TO GET CHARACTERISTICS");
            connected = false;
            return;
        }

        remoteTx->registerForNotify([this] (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
            handleTxCallback(pBLERemoteCharacteristic, pData, length, isNotify);
        });

        Serial.println("SUCCESSFUL CONNECT");

        connected = true;
    }

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
        *data = *(raw + sizeof(uint16_t));
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
    Serial.println("SCAN RECEIVED DEVICE: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.getName() == serverName) { // found the server
        Serial.println("NAME MATCH");
        advertisedDevice.getScan()->stop();
        Serial.println("HERE");
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
    }
}

void BluetoothClient::handleTxCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
    bool isNotify) {
    Serial.println("RECEIVED data: ");
    if (pBLERemoteCharacteristic == remoteTx) {
        Serial.println("RECEIVED TX: ");
        Serial.println(reinterpret_cast<char*>(pData));

        const uint16_t size = *reinterpret_cast<uint16_t *>(pData);
        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            outSerial.write(pData + sizeof(uint16_t), size);
        } else {
            Serial.println("Invalid message size!");
            //this is bad
        }
    }
}

AdvertisingScanHandler::AdvertisingScanHandler(BluetoothClient *client) {
    Serial.println("SCAN HANDLER CREATED!");
    bluetoothClient = client;
}

void AdvertisingScanHandler::onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.println("Advertised Device Found");
    bluetoothClient->handleDeviceCallback(advertisedDevice);
}
