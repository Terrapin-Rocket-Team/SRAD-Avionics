#include "VoltageSensor.h"
#include <Arduino.h>

VoltageSensor::VoltageSensor(int pin, int r1, int r2, const char *name){
    setName(name);
    this->pin = pin;
    this->ratio = 1.0 / (r2 / ( 1.0 * r1 + r2));

    addColumn(DOUBLE, &rawV, "Raw Voltage (V)");
    addColumn(DOUBLE, &realV, "Real Voltage (V)");
}

bool VoltageSensor::init(){
    pinMode(pin, INPUT);
    read();
    return initialized = true;
}

void VoltageSensor::read(){
    rawV = analogRead(pin) * 3.3 / 1023; //values from analogRead range from 0 to 1023 (0 to 3.3v)
    realV = rawV * ratio;
}

bool VoltageSensor::begin(bool unused) {
    return init();
}

void VoltageSensor::update(){
    read();
}