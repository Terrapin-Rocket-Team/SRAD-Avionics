//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothServer.h"

BluetoothServer::BluetoothServer(const std::string &name, Stream &outSerial) : name(name), outSerial(outSerial) {
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
        return true;
    }
    return false;
}

uint16_t BluetoothServer::read(uint8_t *data) {
    uint16_t size = *reinterpret_cast<uint16_t *>(data);
    *data = *(pRxCharacteristic->getData() + sizeof(uint16_t));
    return size;
}


void BluetoothServer::onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {}

void BluetoothServer::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    if (pCharacteristic == pRxCharacteristic) {
        const uint16_t size = *reinterpret_cast<uint16_t *>(pRxCharacteristic->getData());
        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            outSerial.write(pRxCharacteristic->getData(), size);
        } else {
            //this is bad
        }
    }
}

void BluetoothServer::onNotify(BLECharacteristic *pCharacteristic) {}

void BluetoothServer::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) {}
