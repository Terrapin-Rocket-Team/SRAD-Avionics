//
// Created by kking on 7/24/2023.
//

#ifndef BAROMETER_H
#define BAROMETER_H

#include <Arduino.h>


class Barometer {
public:
    virtual ~Barometer() {}; //Virtual descructor. Very important
    virtual void calibrate() = 0; //Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual double get_pressure() = 0;
    virtual double get_temp() = 0;
    virtual double get_temp_f() = 0;
    virtual double get_pressure_atm() = 0;
    virtual double get_rel_alt_ft() = 0;
    virtual String getcsvHeader() = 0;
    virtual String getdataString() = 0;
};


#endif //BAROMETER_H