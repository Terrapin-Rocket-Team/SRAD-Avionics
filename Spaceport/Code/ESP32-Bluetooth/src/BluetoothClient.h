//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTH_CLIENT_H
#define BLUETOOTH_CLIENT_H

#include <BLERemoteCharacteristic.h>
#include <BLERemoteService.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <HardwareSerial.h>  // For serial communication with the MCU (Arduino)

// Server name we want to connect to 
#define bleServerName "ESP32_Avionics"

class BluetoothClient : public BLERemoteCharacteristic {
public:
    BluetoothClient(const std::string& serverName, const std::string& serviceUUID, const std::string& txCharUUID, HardwareSerial& serial);
    // Constructor to create client object using server name connected, server UUID, and sending characteristic UUID
    ~BluetoothClient(); // Destructor

    bool init(); // Method to initialize the client
    bool connectToServer(BLEAddress pAddress);
    bool sendData(const std::string& data);
    
    bool start();   // Start the serial communication
    void update();  // Update and receive serial data from the MCU (Arduino)

    bool send(uint8_t *data, uint16_t length); // Send data to Arduino over serial
    uint16_t read(uint8_t *data);              // Read data from Arduino over serial

    // Callback function for handling advertisement results
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

    // BLERemoteCharacteristic methods
    bool canBroadcast() const { return BLERemoteCharacteristic::canBroadcast(); }
    bool canRead() const { return BLERemoteCharacteristic::canRead(); }
    bool canWrite() const { return BLERemoteCharacteristic::canWrite(); }
    bool canNotify() const { return BLERemoteCharacteristic::canNotify(); }
    bool canIndicate() const { return BLERemoteCharacteristic::canIndicate(); }

private:
    BLEClient* pClient;
    BLERemoteService* pRemoteService;

    std::string serverName;
    std::string serviceUUID;
    std::string rxCharUUID;

    bool initialized = false;

    // Serial communication object for connecting to the Arduino
    HardwareSerial& outSerial;  // Reference to serial output stream (Arduino)

    // Serial buffer for reading data from Arduino
    uint8_t serialBuffer[256];
};

#endif // BLUETOOTH_CLIENT_H
