//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothServer.h>


BluetoothServer server("ESP32 BLE Server", Serial);
void setup() {
    server.start();
}

void loop() {
    server.update(Serial);
}