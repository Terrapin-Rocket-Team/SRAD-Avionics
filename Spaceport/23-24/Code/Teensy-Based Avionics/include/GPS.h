// Placeholder file for the GPS class

#ifndef GPS_H
#define GPS_H

#include <imumaths.h>
#include "Sensor.h"

class GPS : public Sensor
{
public:
    virtual ~GPS(){}; // Virtual descructor. Very important
    virtual double getAlt() = 0;
    virtual imu::Vector<3> getVelocity() = 0;
    virtual imu::Vector<2> getPos() = 0;
    virtual imu::Vector<3> getOriginPos() = 0;
    virtual imu::Vector<3> getDisplace() = 0;
    virtual char *getTimeOfDay() = 0;
    virtual int getFixQual() = 0;
    virtual double getHeading() = 0;
    virtual bool getHasFirstFix() = 0;

    virtual const char *getTypeString() override { return "GPS"; }
    virtual SensorType getType() override { return GPS_; }

};

#endif