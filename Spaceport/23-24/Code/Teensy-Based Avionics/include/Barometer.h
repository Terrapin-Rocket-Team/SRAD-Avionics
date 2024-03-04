#ifndef BAROMETER_H
#define BAROMETER_H

#include "Sensor.h"


class Barometer: public Sensor{
public:
    virtual ~Barometer() {}; //Virtual descructor. Very important
    virtual double get_pressure() = 0;
    virtual double get_temp() = 0;
    virtual double get_temp_f() = 0;
    virtual double get_pressure_atm() = 0;
    virtual double get_rel_alt_ft() = 0;
    virtual double get_rel_alt_m() = 0;

    virtual const char* getTypeString() override { return "Barometer"; }
    virtual SensorType getType() override { return BAROMETER_; }
};


#endif //BAROMETER_H