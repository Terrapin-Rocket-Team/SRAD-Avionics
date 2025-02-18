//
// Created by ramykaddouri on 2/5/25.
//

#include<Arduino.h>
#include<BLEDevice.h>
#include <BluetoothServer.h>

#define SERVER true

#ifdef SERVER
BluetoothServer server(Serial);
#else
//TODO: Implement
#endif

void setup() {
}

void serverLoop() {
    if (Serial.available()) {
        uint8_t messageID = Serial.read();

        switch (messageID) {
            case INIT_MESSAGE:
                std::string name = Serial.readString().c_str();
                server.start(name);
                break;
            case DATA_MESSAGE:
                if (server.isInitialized()) {
                    server.update(Serial);
                }
                break;
            default:
                //unknown message
                break;
        }
    }
}

void clientLoop() {
   //TODO: Implement
}

void loop() {
#ifdef SERVER
    serverLoop();
#else
    clientLoop();
#endif
}
