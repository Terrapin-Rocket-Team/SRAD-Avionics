//
// Created by ramykaddouri on 2/5/25.
//

#include <Arduino.h>
#include <BLEDevice.h>
#include <BluetoothClient.h>
#include <BluetoothServer.h>

// #define SERVER // SERVER is on Avionics, CLIENT is on Airbrake. Comment/uncomment line to change.
#define DEBUG

#ifdef DEV_BRD
#define SERIAL_DBG Serial
#define SERIAL_IN Serial1
#else
#define SERIAL_DBG Serial1
#define SERIAL_IN Serial
#endif

#ifdef SERVER
BluetoothServer server(SERIAL_IN);
#else
BluetoothClient client(SERIAL_IN);
#endif

void setup()
{
    SERIAL_DBG.begin(9600);
    if (&SERIAL_DBG != &SERIAL_IN)
        SERIAL_IN.begin(9600, SERIAL_8N1, 16, 17);
    SERIAL_DBG.println("Hello from ESP32!");
#ifdef DEBUG
#ifdef SERVER
    SERIAL_DBG.println("Server booted!");
    SERIAL_DBG.flush();
    // server.start("ESP32 BLE Server");
#else
    SERIAL_DBG.println("Client booted!");
    SERIAL_IN.println("SERIAL_IN");
    SERIAL_DBG.flush();
    // client.start("ESP32 BLE Server");
#endif
#endif
}

#ifdef SERVER
void serverLoop()
{
    if (SERIAL_IN.available())
    {
        uint8_t messageID = SERIAL_IN.read();
        SERIAL_DBG.println("Received message: " + String(messageID));

        switch (messageID)
        {
        case INIT_MESSAGE:
        {
            if (!server.isInitialized())
            {
                std::string name = SERIAL_IN.readString().c_str();
                SERIAL_DBG.println("Received init message!");
                SERIAL_DBG.println("Server name: " + String(name.c_str()));
                server.start(name);
            }
            else
            {
                SERIAL_DBG.println("WARN: GOT INIT MESSAGE, BUT ALREADY INITIALIZED");
            }
            break;
        }
        case DATA_MESSAGE:
        {
            SERIAL_DBG.println("Received data message");
            SERIAL_DBG.println("Server initialized: " + String(server.isInitialized()));
            SERIAL_DBG.println("SERIAL_IN available: " + String(SERIAL_IN.available()));
            if (server.isInitialized())
            {
                server.update(SERIAL_IN);
            }
            break;
        }
        default:
        {
        }
        }
    }
}
#endif

#ifndef SERVER
void clientLoop()
{
    if (!client.isInitialized())
    {
        if (SERIAL_IN.available())
        {
            const uint8_t messageID = SERIAL_IN.read();
            if (messageID == INIT_MESSAGE)
            {
                std::string name = SERIAL_IN.readStringUntil('\0').c_str();
                SERIAL_DBG.println("Received init message");
                SERIAL_DBG.println("Server name: " + String(name.c_str()) + String(name.size()));
                client.start(name);
            }
        }
    }
    else
    {
        if (!client.isConnected())
        {
            client.update(SERIAL_IN);
        }
        else
        {
            if (SERIAL_IN.available())
            {
                const uint8_t messageID = SERIAL_IN.read();

                switch (messageID)
                {
                case INIT_MESSAGE:
                {
                    SERIAL_DBG.println("WARN: GOT INIT MESSAGE, BUT ALREADY INITIALIZED");
                    break;
                }
                case DATA_MESSAGE:
                {
                    SERIAL_DBG.println("Received data message");
                    if (client.isInitialized())
                    {
                        client.update(SERIAL_IN);
                    }
                    break;
                }
                default:
                {
                    SERIAL_DBG.printf("Unknown message, %hhd\n", messageID);

                    break;
                };
                }
            }
        }
    }
}
#endif

void loop()
{
#ifdef SERVER
    serverLoop();
#else
    clientLoop();
#endif
}
