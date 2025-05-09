#pragma once
#include "Sensors/Sensor.h"

using namespace mmfs;

class VoltageSensor : public Sensor
{
public:
    VoltageSensor(int pin, int r1, int r2, const char *name = "Voltage Sensor");
    bool init() override;
    void read() override;
    bool begin(bool unused = false) override;
    void update() override;

    const SensorType getType() const override { return SensorType::OTHER_; }
    const char *getTypeString() const override { return "Voltage Sensor"; }

    double getRawVoltage() { return rawV; }
    double getRealVoltage() { return realV; }

protected:
    double rawV = 0, realV = 0;
    int pin;
    double ratio;
};