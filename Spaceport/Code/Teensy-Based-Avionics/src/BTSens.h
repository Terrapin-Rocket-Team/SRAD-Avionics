#include "Sensors/Sensor.h"
#include <Arduino.h>
#include "Radio/ESP32BluetoothRadio.h"
using namespace mmfs;

class BTGPS : public Sensor
{
public:
    BTGPS(const char *name, ESP32BluetoothRadio *radio);

    void read() override;

    bool init() override;

private:
    ESP32BluetoothRadio *radio;
    double angle;
};