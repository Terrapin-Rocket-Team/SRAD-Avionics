#ifndef BAROMETER_H
#define BAROMETER_H

#include "Sensor.h"


class Barometer: public Sensor{
public:
    virtual ~Barometer() {}; //Virtual descructor. Very important
    virtual double getPressure() = 0;
    virtual double getTemp() = 0;
    virtual double getTempF() = 0;
    virtual double getPressureAtm() = 0;
    virtual double getRelAltFt() = 0;
    virtual double getRelAltM() = 0;

    virtual const char* getTypeString() override { return "Barometer"; }
    virtual SensorType getType() override { return BAROMETER_; }
};


#endif //BAROMETER_H