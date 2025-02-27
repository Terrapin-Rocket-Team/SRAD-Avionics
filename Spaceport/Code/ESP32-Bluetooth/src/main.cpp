//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothClient.h>
#include <BluetoothServer.h>

// #define SERVER true

#ifdef SERVER
BluetoothServer server(Serial);
#else
//TODO: Implement
BluetoothClient client(Serial);
#endif

void setup() {
    Serial.begin(9600);
    // server.start("ESP32 BLE Server");
    client.start("ESP32 BLE Server");
}

#ifdef SERVER
void serverLoop() {
    if (Serial.available()) {
        uint8_t messageID = Serial.read();

        switch (messageID) {
            case INIT_MESSAGE: {
                std::string name = Serial.readString().c_str();
                server.start(name);
                break;
            }
            case DATA_MESSAGE: {
                if (server.isInitialized()) {
                    server.update(Serial);
                }
                break;
            }
            default: {
                break;
            };
        }
    }
}
#endif

#ifndef SERVER
void clientLoop() {
    client.update(Serial);
    if (client.isConnected()) {
        char buf[] = "Hello From Client!\0";
        client.send(reinterpret_cast<uint8_t *>(buf), strlen(buf) * sizeof(uint8_t));
        Serial.println("Sent data");
    }
    delay(1000);
}
#endif

void loop() {
#ifdef SERVER
    serverLoop();
#else
    clientLoop();
#endif
}
