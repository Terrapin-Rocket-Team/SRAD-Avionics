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
    while (!Serial) {}
#ifdef DEBUG
#ifdef SERVER
    Serial.println("Server Starting!");
    server.start("ESP32 BLE Server");
#else
    Serial.println("Client Starting!");
    Serial.flush();
    client.start("ESP32 BLE Server");
    Serial.println("Client Done Starting!");
    Serial.flush();
#endif
#endif
}

#ifdef SERVER
void serverLoop() {

#ifdef DEBUG
    char buf[] = "Hello World!";
    server.send(reinterpret_cast<uint8_t *>(buf), sizeof(buf));
    delay(500);
#endif
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
        // Serial.println("Client is not connected");
        client.update(Serial);
    } else {
        // Serial.println("Client is connected");
#ifdef DEBUG
        char buf[] = "Hello World!";
        client.send(reinterpret_cast<uint8_t *>(buf), sizeof(buf));
        delay(500);
#endif
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
