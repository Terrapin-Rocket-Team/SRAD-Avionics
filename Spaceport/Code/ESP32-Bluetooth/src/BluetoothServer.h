//
// Created by ramykaddouri on 2/5/25.
//

#ifndef BLUETOOTHSERVER_H
#define BLUETOOTHSERVER_H

#include <string>
#include <BLEDevice.h>

class BluetoothServer : public BLECharacteristicCallbacks {
private:
  const std::string &name;
  BLEServer *pServer = nullptr;
  BLEService *pService = nullptr;
  BLECharacteristic *pCharacteristic = nullptr;
  BLEAdvertising *pAdvertising = nullptr;
public:
  explicit BluetoothServer(const std::string &name);
  ~BluetoothServer() override;

  bool start();

  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;
  void onNotify(BLECharacteristic *pCharacteristic) override;
  void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) override;
};



#endif //BLUETOOTHSERVER_H
