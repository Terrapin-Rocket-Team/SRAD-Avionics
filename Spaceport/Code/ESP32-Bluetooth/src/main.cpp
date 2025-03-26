//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothClient.h>
#include <BluetoothServer.h>

#define SERVER
#define DEBUG

#ifdef SERVER
BluetoothServer server(Serial1);
#else
BluetoothClient client(Serial);
#endif

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("Hello from ESP32!");
#ifdef DEBUG
#ifdef SERVER
    Serial.println("Server booted!");
    Serial.flush();
    // server.start("ESP32 BLE Server");
#else
    Serial.println("Client booted!");
    Serial.flush();
    // client.start("ESP32 BLE Server");
#endif
#endif
}

#ifdef SERVER
void serverLoop() {
    if (Serial1.available()) {
        uint8_t messageID = Serial1.read();
        Serial.println("Received message: " + String(messageID));

        switch (messageID) {
            case INIT_MESSAGE: {
                if (!server.isInitialized()) {
                    std::string name = Serial1.readString().c_str();
                    Serial.println("Received init message!");
                    Serial.println("Server name: " + String(name.c_str()));
                    Serial.println("WARN: GOT INIT MESSAGE, BUT ALREADY INITIALIZED");
                } else {
                    Serial.println("WARN: R");
                }
                break;
            }
            case DATA_MESSAGE: {
                Serial.println("Received data message");
                if (server.isInitialized()) {
                    server.update(Serial1);
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
    if (!client.isInitialized()) {
        if (Serial1.available()) {
            const uint8_t messageID = Serial1.read();
            if (messageID == INIT_MESSAGE) {
                std::string name = Serial1.readString().c_str();
                Serial.println("Received init message");
                Serial.println("Server name: " + String(name.c_str()));
                client.start(name);
            }
        }
    } else {
        if (!client.isConnected()) {
            client.update(Serial1);
        } else {
            if (Serial1.available()) {
                const uint8_t messageID = Serial1.read();

                switch (messageID) {
                    case INIT_MESSAGE: {
                        Serial.println("WARN: GOT INIT MESSAGE, BUT ALREADY INITIALIZED");
                        break;
                    }
                    case DATA_MESSAGE: {
                        Serial.println("Received data message");
                        if (client.isInitialized()) {
                            client.update(Serial1);
                        }
                        break;
                    }
                    default: {
                        Serial.println("Unknown message");
                        break;
                    };
                }
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
