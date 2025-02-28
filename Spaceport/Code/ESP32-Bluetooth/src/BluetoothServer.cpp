//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothServer.h"

#include <HardwareSerial.h>

BluetoothServer::BluetoothServer(Stream &outSerial) : outSerial(outSerial) {}

BluetoothServer::~BluetoothServer() {
    BLEDevice::deinit(true);
    initialized = false;
}

bool BluetoothServer::start(const std::string &name) {
    this->name = name;

    BLEDevice::init(name);

    pServer = BLEDevice::createServer();
    //TODO: There should probably be a better way to do these UUIDs
    pService = pServer->createService("cba1d466-344c-4be3-ab3f-189f80dd7518");
    pRxCharacteristic = pService->createCharacteristic(
        "9c15bf50-4a60-40",
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic = pService->createCharacteristic(
        "5159c5ac-b886-42",
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    pTxCharacteristic->setValue("TX");
    pRxCharacteristic->setValue("RX");

    pRxCharacteristic->setCallbacks(this);
    pTxCharacteristic->setCallbacks(this);

    if (pService != nullptr) {
        pService->start();
        pAdvertising = pServer->getAdvertising();
        pAdvertising->start();

        initialized = true;
        return true;
    }

    initialized = false;
    return false;
}

void BluetoothServer::update(Stream &inputSerial) {
    if (!inputSerial.available()) return;

    uint16_t size = 0;
    inputSerial.readBytes(reinterpret_cast<char *>(&size), sizeof(uint16_t));

    if (size != 0 && size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
        uint8_t buffer[size];
        inputSerial.readBytes(buffer, size);
        send(buffer, size);
    }
}



bool BluetoothServer::send(uint8_t* data, uint16_t size) {
    if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
        //Encode size as uint_16t, then data
        uint8_t buf[size+sizeof(uint16_t)];
        buf[0] = size;
        memcpy(&buf[sizeof(uint16_t)], data, size);

        pTxCharacteristic->setValue(data, size+sizeof(uint16_t));
        pTxCharacteristic->notify();
        return true;
    }
    return false;
}

uint16_t BluetoothServer::read(uint8_t *data) {
    uint16_t size = *reinterpret_cast<uint16_t *>(data);
    *data = *(pRxCharacteristic->getData() + sizeof(uint16_t));
    return size;
}

bool BluetoothServer::isInitialized() const {
    return initialized;
}


void BluetoothServer::onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
}

void BluetoothServer::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    if (pCharacteristic == pRxCharacteristic) {
        const uint16_t size = *reinterpret_cast<uint16_t *>(pRxCharacteristic->getData());
        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            outSerial.write(pRxCharacteristic->getData() + sizeof(uint16_t), size);
        } else {
            //this is bad
        }
    }
}

void BluetoothServer::onNotify(BLECharacteristic *pCharacteristic) {}

void BluetoothServer::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) {}
