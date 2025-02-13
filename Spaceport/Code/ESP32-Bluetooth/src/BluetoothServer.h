//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTHSERVER_H
#define BLUETOOTHSERVER_H

#include <string>
#include <BLEDevice.h>
#include <Stream.h>

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
  Stream &outSerial;
public:
  explicit BluetoothServer(const std::string &namem, Stream &outSerial);
  ~BluetoothServer() override;

  bool start();
  void update(Stream& inputSerial);

  bool send(uint8_t *data, uint16_t length);
  uint16_t read(uint8_t *data);


  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onNotify(BLECharacteristic *pCharacteristic) override;
  void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) override;
};



#endif //BLUETOOTHSERVER_H
