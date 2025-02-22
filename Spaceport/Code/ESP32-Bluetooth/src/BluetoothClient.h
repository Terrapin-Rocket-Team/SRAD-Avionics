//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTH_CLIENT_H
#define BLUETOOTH_CLIENT_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Stream.h>
#include "MessageCodes.h"

class BluetoothClient {
public:
    BluetoothClient(Stream& outSerial);
    // Constructor to create client object using server name connected, server UUID, and sending characteristic UUID
    ~BluetoothClient(); // Destructor

    bool start(const std::string& serverName);   // Start the serial communication
    void update(Stream& inputSerial);  // Update and receive serial data from the MCU (Arduino)

    bool send(uint8_t *data, uint16_t length); // Send data to Arduino over serial
    uint16_t read(uint8_t *data);              // Read data from Arduino over serial

    bool isInitialized();
    bool isConnected();

    void handleDeviceCallback(BLEAdvertisedDevice advertisedDevice);
    void handleTxCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
private:
    BLEClient* pClient = nullptr;
    BLEAddress* pServerAddress = nullptr;
    BLERemoteService* pRemoteService = nullptr;

    std::string serverName;

    BLERemoteCharacteristic* remoteTx = nullptr;
    BLERemoteCharacteristic* remoteRx = nullptr;

    bool initialized = false;
    bool connected = false;

    // Serial communication object for connecting to the Arduino
    Stream& outSerial;  // Reference to serial output stream (Arduino)
};


class AdvertisingScanHandler : public BLEAdvertisedDeviceCallbacks {
private:
    BluetoothClient& bluetoothClient;
public:
    explicit AdvertisingScanHandler(BluetoothClient& client);
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
};

#endif // BLUETOOTH_CLIENT_H
