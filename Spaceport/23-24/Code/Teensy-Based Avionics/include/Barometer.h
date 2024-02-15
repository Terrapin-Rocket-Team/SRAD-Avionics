#ifndef BAROMETER_H
#define BAROMETER_H

#include <Arduino.h>
#include "Sensor.h"

class Barometer : public Sensor
{
public:
    virtual ~Barometer(){};        // Virtual descructor. Very important
    virtual void initialize() = 0; // Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual double get_pressure() = 0;
    virtual double get_temp() = 0;
    virtual double get_temp_f() = 0;
    virtual double get_pressure_atm() = 0;
    virtual double get_rel_alt_ft() = 0;
    virtual void *get_data() = 0;
    virtual char **getcsvHeader() = 0;
    virtual char **getdataString() = 0;
    virtual String getStaticDataString() = 0;
};

#endif // BAROMETER_H