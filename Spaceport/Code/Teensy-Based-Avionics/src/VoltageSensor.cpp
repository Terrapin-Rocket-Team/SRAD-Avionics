#include "VoltageSensor.h"
#include <Arduino.h>

VoltageSensor::VoltageSensor(int pin, int r1, int r2, const char *name) : Sensor("Voltage Sensor", name)
{
    this->pin = pin;
    this->ratio = 1.0 / (r2 / (1.0 * r1 + r2));

    addColumn(DOUBLE, &rawV, "Raw Voltage (V)");
    addColumn(DOUBLE, &realV, "Real Voltage (V)");
}

bool VoltageSensor::init()
{
    pinMode(pin, INPUT);
    read();
    return initialized = true;
}

bool VoltageSensor::read()
{
    result = analogRead(pin);
    rawV = result * 3.3 / 1023 / 3.3; // values from analogRead range from 0 to 1023 (0 to 3.3v)
    realV = rawV * ratio;
    return true;
}