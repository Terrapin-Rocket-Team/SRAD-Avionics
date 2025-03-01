//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothClient.h>
#include <BluetoothServer.h>

// #define SERVER
#define DEBUG

#ifdef SERVER
BluetoothServer server(Serial);
#else
BluetoothClient client(Serial);
#endif

void setup() {
#ifdef DEBUG
    Serial.begin(9600);
#endif

#ifdef SERVER
    server.start("ESP32 BLE Server");
    #ifdef DEBUG
    Serial.println("Server started");
    #endif
#else
    #ifdef DEBUG
    Serial.println("Trying to start client");
    #endif
    client.start("ESP32 BLE Server");
    #ifdef DEBUG
    Serial.println("Client started");
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
                #ifdef DEBUG
                Serial.println("Received INIT_MESSAGE");
                #endif
                break;
            }
            case DATA_MESSAGE: {
                if (server.isInitialized()) {
                    server.update(Serial);
                    #ifdef DEBUG
                    Serial.println("Received DATA_MESSAGE");
                    #endif
                }
                break;
            }
            default: {
                #ifdef DEBUG
                Serial.println("Received unknown message");
                #endif
                break;
            };
        }
    }
    #ifdef DEBUG
    Serial.println("Server loop");
    #endif
}
#endif

#ifndef SERVER
void sendHelloWorld() {
    if (client.isConnected()) {
        const char* message = "Hello World";
        client.send((uint8_t*)message, strlen(message));
        #ifdef DEBUG
        Serial.println("Sent Hello World");
        #endif
    }
}

void clientLoop() {
    if (!client.isConnected()) {
        client.update(Serial);
        #ifdef DEBUG
        Serial.println("Client not connected");
        #endif
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
        #ifdef DEBUG
        sendHelloWorld(); // Send "Hello World" message
        delay(1000); // Delay to avoid flooding the messages
        #endif
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
