// Placeholder file for the LightSensor class

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Arduino.h>

class LightSensor
{
public:
    virtual ~LightSensor(){};     // Virtual descructor. Very important
    virtual void calibrate() = 0; // Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual double get_pressure() = 0;
    virtual double get_temp() = 0;
    virtual double get_temp_f() = 0;
    virtual double get_pressure_atm() = 0;
    virtual double get_rel_alt_ft() = 0;
    virtual char **getcsvHeader() = 0;
    virtual char **getdataString() = 0;
};

#endif