//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTH_CLIENT_H
#define BLUETOOTH_CLIENT_H

//server name we want to connect to 
#define bleServerName "ESP32_Avionics"

/*will be put into use once I know the rX characteristic object in the server esp
static BLEUUID bmeServiceUUID()
*/


#include <BLERemoteCharacteristic.h>
#include <BLERemoteService.h>


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <HardwareSerial.h>  // For serial communication with the MCU (Arduino)

class BluetoothClient : public BLEAdvertisedDeviceCallbacks { //inheriting from advertised devices callback which will help me scan for and manage servers;
    //used for connecting to other servers so i need that
    //didn't import or inherit BLEremotecharacteristic because i don't really care if the object on the other end changes or if any data inside the chracteristic object changes 
    
public:
    BluetoothClient(const std::string& serverName, const std::string& serviceUUID, const std::string& txCharUUID, HardwareSerial& serial);
    //constructor to create client object using server name connected, server uuid, and sending characteristic uuid
    ~BluetoothClient(); //destructor

    void init();
    //method to initialize the client
    bool connectToServer(BLEAddress pAddress);
    bool sendData(const std::string& data);

    bool start();   // Start the serial communication
    void update();  // Update and receive serial data from the MCU (Arduino)

    bool send(uint8_t *data, uint16_t length); // Send data to Arduino over serial
    uint16_t read(uint8_t *data);              // Read data from Arduino over serial

    // Callback function for handling advertisement results
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

private:
    BLEClient* pClient;
    BLERemoteService* pRemoteService;
    BLERemoteCharacteristic* txCharacteristic;

    std::string serverName;
    std::string serviceUUID;
    std::string rxCharUUID;

    // Serial communication object for connecting to the Arduino
    HardwareSerial& outSerial;  // Reference to serial output stream (Arduino)

    // Serial buffer for reading data from Arduino
    uint8_t serialBuffer[256];
};

#endif // BLUETOOTH_CLIENT_H
