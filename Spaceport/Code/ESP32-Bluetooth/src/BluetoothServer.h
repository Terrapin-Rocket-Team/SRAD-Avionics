//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTHSERVER_H
#define BLUETOOTHSERVER_H

#include <string>
#include <BLEDevice.h>
#include <Message.h>

#define MAX_MESSAGE_SIZE 512

class BluetoothServer : public BLECharacteristicCallbacks {
private:
  const std::string &name;
  BLEServer *pServer = nullptr;
  BLEService *pService = nullptr;
  // Max size that a characteristic can hold is 512 bytes
  // might have to develop a system of splitting data
  // over multiple characteristics if necessary.
  BLECharacteristic *pRxCharacteristic = nullptr;
  BLECharacteristic *pTxCharacteristic = nullptr;
  BLEAdvertising *pAdvertising = nullptr;
public:
  explicit BluetoothServer(const std::string &name);
  ~BluetoothServer() override;

  bool start();

  bool send(Message& message);
  int read(Message &message);

  bool send(uint8_t *data, size_t length);
  int read(uint8_t *data, size_t length);

  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onNotify(BLECharacteristic *pCharacteristic) override;
  void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) override;
};



#endif //BLUETOOTHSERVER_H
