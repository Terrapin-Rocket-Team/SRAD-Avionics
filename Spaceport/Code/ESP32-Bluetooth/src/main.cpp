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
    Seria.flush();
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
                // std::string name = Serial1.readString().c_str();
                char buf[128] = {'\0'};
                Serial1.readBytesUntil('\0', buf, sizeof(buf));
                Serial.print("Received init message with server name: ");
                Serial.println(buf);

                server.start(buf);
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
    if (!client.isConnected()) {
        Serial.println("Client is not connected");
        client.update(Serial1);
    } else {
        Serial.println("Client is connected");
        if (Serial1.available()) {
            uint8_t messageID = Serial1.read();

            switch (messageID) {
                case INIT_MESSAGE: {
                    std::string name = Serial1.readString().c_str();
                    client.start(name);
                    break;
                }
                case DATA_MESSAGE: {
                    if (client.isInitialized()) {
                        client.update(Serial1);
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
