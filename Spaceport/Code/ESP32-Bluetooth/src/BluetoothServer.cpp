//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothServer.h"

#include <Client.h>
#include <HardwareSerial.h>

BluetoothServer::BluetoothServer(Stream &outSerial) : outSerial(outSerial) {
    clientConnectionHandler = new ClientConnectionHandler(this);
}

BluetoothServer::~BluetoothServer() {
    BLEDevice::deinit(true);
    initialized = false;
}

bool BluetoothServer::start(const std::string &name) {
    this->name = name;

    BLEDevice::init(name);
    BLEDevice::setMTU(128);

    Serial.print("Server Device initialized with name: ");
    Serial.println(name.c_str());

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
    pServer->setCallbacks(clientConnectionHandler);

    if (pService != nullptr) {
        pService->start();
        pAdvertising = pServer->getAdvertising();
        pAdvertising->start();

        Serial.println("Server Advertising started!");

        initialized = true;
        outSerial.write(STATUS_MESSAGE);
        outSerial.write(1);
        outSerial.flush();
        return true;
    }

    initialized = false;
    return false;
}

void BluetoothServer::update(Stream &inputSerial) {
    while (!inputSerial.available()) {}

    uint16_t size = 0;
    inputSerial.readBytes(reinterpret_cast<char *>(&size), sizeof(uint16_t));

    Serial.println("Received message to send with size: " + String(size));

    if (size != 0 && size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
        uint8_t buffer[size];
        inputSerial.readBytes(buffer, size);
        Serial.println("Read message!");

        Serial.println("Message Content: ");
        for (int i = 0; i < size; i++) {
            Serial.printf("%c", buffer[i]);
        }
        Serial.println("");

        send(buffer, size);
        Serial.println("Sent message!");
    }
}

void BluetoothServer::startAdvertising() {
    if (pService != nullptr) {
        pServer->startAdvertising();
    }
}

bool BluetoothServer::send(uint8_t* data, uint16_t size) { //how do we send via serial? need pins tx0 rx0
    Serial.println("Sending message of size: " + String(size));
    if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
        //Encode size as uint_16t, then data
        uint8_t buf[size+sizeof(uint16_t)];

        memcpy(buf, &size, sizeof(uint16_t));
        memcpy(buf + sizeof(uint16_t), data, size);


        Serial.print("\t> Send Buffer HEX: ");
        for (int i = 0; i < size + sizeof(uint16_t); i++) {
            Serial.printf("%X", buf[i]);
        }
        Serial.println("");

        pTxCharacteristic->setValue(buf, size+sizeof(uint16_t));
        pTxCharacteristic->notify();
        Serial.println("Sent message and notified clients!");
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
        Serial.println("Server received data on RX: ");

        const uint16_t size = *reinterpret_cast<uint16_t *>(pRxCharacteristic->getData());
        Serial.println("Message size: " + String(size));
        if (size <= MAX_MESSAGE_SIZE-sizeof(uint16_t)) {
            const uint8_t *data = pRxCharacteristic->getData() + sizeof(uint16_t);
            Serial.println("Message content");
            for (int i = 0; i < size; i++) {
                Serial.print(static_cast<char>(data[i]));
            }
            Serial.println("");
            outSerial.write(DATA_MESSAGE);
            outSerial.write(pRxCharacteristic->getData(), size + sizeof(uint16_t));
        } else {
            Serial.println("ERROR: Invalid message size!");
        }
    }
}

void BluetoothServer::onNotify(BLECharacteristic *pCharacteristic) {}

void BluetoothServer::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) {}

ClientConnectionHandler::ClientConnectionHandler(BluetoothServer *pServer) {
    this->pServer = pServer;
}

void ClientConnectionHandler::onConnect(BLEServer *pServer) {
    Serial.println("Client connection established!");
    pServer->updatePeerMTU(pServer->getConnId(), 128);
    Serial.println("Peer MTU: " + String(pServer->getPeerMTU(pServer->getConnId())));
    // Serial.println("onConnect, restarting advertising");
    // pServer->startAdvertising(); //restart advertising when a device connects
}

void ClientConnectionHandler::onDisconnect(BLEServer *pServer) {
    Serial.println("Client disconnected!");
    // Serial.println("onDisconnect, restarting advertising");
    pServer->startAdvertising(); //restart advertising when device connectes
}


