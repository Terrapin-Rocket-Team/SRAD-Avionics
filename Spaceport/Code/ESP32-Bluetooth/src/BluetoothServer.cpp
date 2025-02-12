//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothServer.h"

BluetoothServer::BluetoothServer(const std::string &name) : name(name) {
    BLEDevice::init(name);

    pServer = BLEDevice::createServer();
    //TODO: There should probably be a better way to do these UUIDs
    pService = pServer->createService(name + "-service");
    pRxCharacteristic = pService->createCharacteristic(
        name + "-charRX",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic = pService->createCharacteristic(
        name + "-charTX",
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(this);
    pTxCharacteristic->setCallbacks(this);
}

BluetoothServer::~BluetoothServer() {
    BLEDevice::deinit(true);
}

bool BluetoothServer::start() {
    if (pService != nullptr) {
        pService->start();
        pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
        return true;
    }

    return false;
}

bool BluetoothServer::send(Message &message) {
    uint8_t buf[message.size];
    message.get(buf);
    send(buf, message.size);
}

int BluetoothServer::read(Message &message) {
    uint8_t buf[MAX_MESSAGE_SIZE];
    *buf = *pRxCharacteristic->getData();
    message.fill(buf, MAX_MESSAGE_SIZE);
}

bool BluetoothServer::send(uint8_t* data, size_t length) {
    if (length <= MAX_MESSAGE_SIZE) {
        pTxCharacteristic->setValue(data, length);
        return true;
    }
    return false; //placeholder
}

int BluetoothServer::read(uint8_t *data, size_t length) {
    *data = *pRxCharacteristic->getData();
    return 1; //placeholder
}


void BluetoothServer::onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    //TODO: implement
}

void BluetoothServer::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    //received some data in RX
    if (pCharacteristic == pRxCharacteristic) {
        Message message;
        read(message);
        uint8_t buf[message.size];
        message.get(buf);

        Serial.write(buf, message.size);
    }
}

void BluetoothServer::onNotify(BLECharacteristic *pCharacteristic) {
    //TODO: implement
}

void BluetoothServer::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) {
    //TODO: implement
}
