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
        name + "-characteristic0",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic = pService->createCharacteristic(
        name + "-characteristic1",
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

void BluetoothServer::onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    //TODO: implement
}

void BluetoothServer::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    //TODO: implement
}

void BluetoothServer::onNotify(BLECharacteristic *pCharacteristic) {
    //TODO: implement
}

void BluetoothServer::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) {
    //TODO: implement
}
