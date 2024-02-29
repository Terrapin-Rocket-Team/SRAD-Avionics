// Placeholder file for the GPS class

#ifndef GPS_H
#define GPS_H

#include <imumaths.h>
#include "Sensor.h"

class GPS : public Sensor
{
public:
    virtual ~GPS(){}; // Virtual descructor. Very important
    virtual double get_alt() = 0;
    virtual imu::Vector<3> get_velocity() = 0;
    virtual imu::Vector<2> get_pos() = 0;
    virtual imu::Vector<3> get_origin_pos() = 0;
    virtual imu::Vector<3> get_displace() = 0;
    virtual double get_gps_time() = 0;
    virtual int get_fix_qual() = 0;
    virtual double get_heading() = 0;

    virtual const char *getTypeString() override { return "GPS"; }
    virtual SensorType getType() override { return GPS_; }

};

#endif