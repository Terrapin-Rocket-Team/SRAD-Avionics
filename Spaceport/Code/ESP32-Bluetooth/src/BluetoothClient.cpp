//
// Created by ramykaddouri on 2/5/25.
//

#include "BluetoothClient.h"
#include "esp_gap_ble_api.h"

#include <HardwareSerial.h>

BluetoothClient::BluetoothClient(Stream &outSerial, Stream &dbgSerial) : outSerial(outSerial), dbgSerial(dbgSerial) {}

BluetoothClient::~BluetoothClient()
{
    BLEDevice::deinit();
}

bool BluetoothClient::start(const std::string &serverName)
{ // is this the start for bluetooth or serial communication??

    // add serial communication code because currently it's all bluetooth
    this->serverName = "AVIONICS";
    BLEDevice::init("");
    dbgSerial.println("Initializing client device with target server " + String(serverName.c_str()));
    dbgSerial.println(BLEDevice::getMTU());
    pClient = BLEDevice::createClient();
    BLEScan *pBLEScan = BLEDevice::getScan();
    pScanHandler = new AdvertisingScanHandler(this);
    pBLEScan->setAdvertisedDeviceCallbacks(pScanHandler);
    pBLEScan->setActiveScan(true);
    // pBLEScan->setInterval(30);
    // pBLEScan->setWindow(29);

    pBLEScan->start(120);
    // dbgSerial.println("BLE scan done!");
    dbgSerial.println("Starting BLE scan, client successfully initialized!");
    initialized = true;
    return initialized;
}

void BluetoothClient::update(Stream &inputSerial)
{

    if (!connected && pServerAddress != nullptr)
    {
        dbgSerial.println("Attempting to connect to " + String(pServerAddress->toString().c_str()));
        pClient->connect(*pServerAddress);

        pRemoteService = pClient->getService("cba1d466-344c-4be3-ab3f-189f80dd7518");
        if (pRemoteService == nullptr)
        {
            // fail
            dbgSerial.println("Failed to acquire remote service!");
            connected = false;
            return;
        }

        remoteRx = pRemoteService->getCharacteristic("9c15bf50-4a60-40"); // server's rx
        remoteTx = pRemoteService->getCharacteristic("5159c5ac-b886-42"); // server's tx

        if (remoteRx == nullptr || remoteTx == nullptr)
        {
            dbgSerial.println("Failed to acquire remote characteristics!");
            connected = false;
            return;
        }
        esp_ble_conn_update_params_t conn_params = {};
        memcpy(conn_params.bda, pServerAddress->getNative(), sizeof(conn_params.bda));
        conn_params.min_int = 0x400; // 40 ms
        conn_params.max_int = 0x400; // 60 ms
        conn_params.latency = 0;
        conn_params.timeout = 400; // 4s supervision timeout

        // esp_err_t err = esp_ble_gap_set_prefer_conn_params(conn_params.bda, conn_params.min_int, conn_params.max_int, conn_params.latency, conn_params.timeout);
        // dbgSerial.println("Requested connection interval update, result: " + String(err));

        remoteTx->registerForNotify([this](BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
                                    { handleTxCallback(pBLERemoteCharacteristic, pData, length, isNotify); });
        dbgSerial.println(pClient->setMTU(256));
        connected = true;
        dbgSerial.println("Client successfully connected to server " + String(serverName.c_str()));
        dbgSerial.println("Writing status message...");
        outSerial.write(STATUS_MESSAGE);
        outSerial.write(1);
        outSerial.flush();
        dbgSerial.println("Status message sent!");
    }

    if (initialized && connected)
    {
        char asdf[] = "KQ4TCN>ALL,WIDE1-1:!MNN!!NN!!\\ !!\"1#Q$!!#j!!!!!\\(";
        send((uint8_t *)asdf, sizeof(asdf) - 1);
        delay(400);
        //     uint16_t size = 0;
        //     inputSerial.readBytes(reinterpret_cast<char *>(&size), sizeof(uint16_t));

        //     dbgSerial.println("Received message to send with size: " + String(size));

        //     if (size != 0 && size <= MAX_MESSAGE_SIZE - sizeof(uint16_t))
        //     {
        //         uint8_t buffer[size];
        //         inputSerial.readBytes(buffer, size);
        //         dbgSerial.println("Sending the following message content: ");
        //         for (int i = 0; i < size; i++)
        //         {
        //             dbgSerial.print((char)buffer[i]);
        //         }
        //         dbgSerial.println("");
        //         send(buffer, size);
        //     }
    }
}

bool BluetoothClient::send(uint8_t *data, uint16_t length)
{
    if (remoteRx->canWrite())
    {
        uint8_t buf[length + sizeof(uint16_t)];
        memcpy(buf, &length, sizeof(uint16_t));
        memcpy(buf + sizeof(uint16_t), data, length);

        dbgSerial.println("Writing data to server RX");
        remoteRx->writeValue(buf, length + sizeof(uint16_t));
        dbgSerial.write(buf, length + sizeof(uint16_t));
        return true;
    }
    return false;
}

uint16_t BluetoothClient::read(uint8_t *data)
{
    if (remoteRx->canRead())
    {
        dbgSerial.println("Reading data from server RX");
        uint8_t *raw = remoteRx->readRawData();

        uint16_t size = *reinterpret_cast<uint16_t *>(raw);
        *data = *(raw + sizeof(uint16_t));
        return size;
    }

    return 0;
}

bool BluetoothClient::isInitialized()
{
    return initialized;
}

bool BluetoothClient::isConnected()
{
    return connected;
}

void BluetoothClient::handleDeviceCallback(BLEAdvertisedDevice advertisedDevice)
{
    dbgSerial.println("Device Detected: " + String(advertisedDevice.toString().c_str()));
    if (advertisedDevice.getName() == serverName)
    { // found the server
        dbgSerial.println("Server Found! Stopping scan...");
        advertisedDevice.getScan()->stop();
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
    }
}

void BluetoothClient::handleTxCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
                                       bool isNotify)
{
    if (pBLERemoteCharacteristic == remoteTx)
    {
        dbgSerial.print("Client received data on server TX: ");
        const uint16_t size = *reinterpret_cast<uint16_t *>(pData);
        dbgSerial.println("Received data from server TX size: " + String(size));
        dbgSerial.println("Message content: ");
        for (int i = 0; i < size; i++)
        {
            dbgSerial.print(pData[i]);
        }
        dbgSerial.println("");

        if (size <= MAX_MESSAGE_SIZE - sizeof(uint16_t))
        {
            dbgSerial.println("Sending data received from server to serial");
            outSerial.write(DATA_MESSAGE);
            outSerial.write((uint8_t *)&size, 2);
            outSerial.write(pData + sizeof(uint16_t), size);
        }
        else
        {
            dbgSerial.print("Invalid message size!");
        }
    }
}

AdvertisingScanHandler::AdvertisingScanHandler(BluetoothClient *client)
{
    bluetoothClient = client;
}

void AdvertisingScanHandler::onResult(BLEAdvertisedDevice advertisedDevice)
{
    bluetoothClient->handleDeviceCallback(advertisedDevice);
}
