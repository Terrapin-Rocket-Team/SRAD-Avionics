//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothClient.h>
#include <BluetoothServer.h>

#define SERVER
// #define DEBUG

#ifdef SERVER
BluetoothServer server(Serial);
#else
BluetoothClient client(Serial);
#endif

void setup() {
    Serial.begin(9600);
#ifdef DEBUG
#ifdef SERVER
    server.start("ESP32 BLE Server");
#else
    client.start("ESP32 BLE Server");
#endif
#endif
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
    if (!client.isConnected()) {
        client.update(Serial);
    } else {
        if (Serial.available()) {
            uint8_t messageID = Serial.read();

            switch (messageID) {
                case INIT_MESSAGE: {
                    std::string name = Serial.readString().c_str();
                    client.start(name);
                    break;
                }
                case DATA_MESSAGE: {
                    if (client.isInitialized()) {
                        client.update(Serial);
                    }
                    break;
                }
                default: {
                    break;
                };
            }
        }
    }
}
#endif

void loop() {
#ifdef SERVER
    serverLoop();
#else
    clientLoop();
#endif
}
