//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ReceiveCallback : public BLECharacteristicCallbacks {
    // the characteristic was written to (ie data was received from
    // another device)
    void onWrite(BLECharacteristic *characteristic) {
        //write the data to serial for now
        Serial.write(characteristic->getValue().c_str());
    }
};

void setup() {
    Serial.begin(9600);
    BLEDevice::init("ESP32 Bluetooth Module");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *firstCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );

    // firstCharacteristic->setCallbacks()
    firstCharacteristic->setValue("Hello from ESP32");
    firstCharacteristic->setCallbacks(new ReceiveCallback());

    pService->start();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
}

void loop() {
    if (Serial.available()) {
        const uint8_t byte = Serial.read();
        //Do some stuff here to handle incoming messages
    }
}