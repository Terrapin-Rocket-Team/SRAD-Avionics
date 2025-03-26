//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothClient.h"

#include <HardwareSerial.h>

BluetoothClient::BluetoothClient(Stream &outSerial) : outSerial(outSerial) {}

BluetoothClient::~BluetoothClient() {
    BLEDevice::deinit();
}

bool BluetoothClient::start(const std::string &serverName) { //is this the start for bluetooth or serial communication??

    //add serial communication code because currently it's all bluetooth
    this->serverName = serverName;
    BLEDevice::init("");
    Serial.println("Initializing client device with target server " + String(serverName.c_str()));

    pClient = BLEDevice::createClient();

    BLEScan *pBLEScan = BLEDevice::getScan();
    pScanHandler = new AdvertisingScanHandler(this);
    pBLEScan->setAdvertisedDeviceCallbacks(pScanHandler);
    pBLEScan->setActiveScan(true);
    // pBLEScan->setInterval(30);
    // pBLEScan->setWindow(29);

    pBLEScan->start(120);
    // Serial.println("BLE scan done!");
    Serial.println("Starting BLE scan, client successfully initialized!");
    initialized = true;
    return initialized;
}

void BluetoothClient::update(Stream& inputSerial) { 
    if (!connected && pServerAddress != nullptr) {
        Serial.println("Attempting to connect to " + String(pServerAddress->toString().c_str()));
        pClient->connect(*pServerAddress);

        pRemoteService = pClient->getService("cba1d466-344c-4be3-ab3f-189f80dd7518");
        if (pRemoteService == nullptr) {
            //fail
            Serial.println("Failed to acquire remote service!");
            connected = false;
            return;
        }

        remoteRx = pRemoteService->getCharacteristic("9c15bf50-4a60-40"); //server's rx
        remoteTx = pRemoteService->getCharacteristic("5159c5ac-b886-42"); //server's tx

        if (remoteRx == nullptr || remoteTx == nullptr) {
            Serial.println("Failed to acquire remote characteristics!");
            connected = false;
            return;
        }
        remoteTx->registerForNotify([this] (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
            handleTxCallback(pBLERemoteCharacteristic, pData, length, isNotify);
        });

        connected = true;
        Serial.println("Client successfully connected to server " + String(serverName.c_str()));
        Serial.println("Writing status message...");
        outSerial.write(STATUS_MESSAGE);
        outSerial.write(1);
        outSerial.flush();
        Serial.println("Status message sent!");
    }

    if (initialized && connected) {
        if (!inputSerial.available()) return;

        uint16_t size = 0;
        inputSerial.readBytes(reinterpret_cast<char *>(&size), sizeof(uint16_t));

        Serial.println("Received message to send with size: " + String(size));

        if (size != 0 && size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            uint8_t buffer[size];
            inputSerial.readBytes(buffer, size);
            Serial.println("Sending the following message content: ");
            for (int i = 0; i < size; i++) {
                Serial.print(buffer[i]);
            }
            Serial.println("");
            send(buffer, size);
        }
    }
}

bool BluetoothClient::send(uint8_t *data, uint16_t length) {
    if (remoteRx->canWrite()) {
        Serial.println("Writing data to server RX");
        remoteRx->writeValue(data, length);
        return true;
    }
    return false;
}

uint16_t BluetoothClient::read(uint8_t *data) {
    if (remoteRx->canRead()) {
        Serial.println("Reading data from server RX");
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
    Serial.println("Device Detected: " + String(advertisedDevice.toString().c_str()));
    if (advertisedDevice.getName() == serverName) { // found the server
        Serial.println("Server Found! Stopping scan...");
        advertisedDevice.getScan()->stop();
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
    }
}

void BluetoothClient::handleTxCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
    bool isNotify) {
    if (pBLERemoteCharacteristic == remoteTx) {
        Serial.print("Client received data on server TX: ");
        const uint16_t size = *reinterpret_cast<uint16_t *>(pData);
        Serial.println("Received data from server TX size: " + String(size));
        Serial.println("Message content: ");
        for (int i = 0; i < size; i++) {
            Serial.print(pData[i]);
        }
        Serial.println("");

        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            Serial.println("Sending data received from server to serial");
            outSerial.write(DATA_MESSAGE);
            outSerial.write(size);
            outSerial.write(pData + sizeof(uint16_t), size);
        } else {
            Serial.print("Invalid message size!");
        }
    }
}

AdvertisingScanHandler::AdvertisingScanHandler(BluetoothClient *client) {
    bluetoothClient = client;
}

void AdvertisingScanHandler::onResult(BLEAdvertisedDevice advertisedDevice) {
    bluetoothClient->handleDeviceCallback(advertisedDevice);
}
