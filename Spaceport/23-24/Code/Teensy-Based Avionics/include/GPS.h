// Placeholder file for the GPS class

#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <imumaths.h>


class GPS {
public:
    virtual ~GPS() {}; //Virtual descructor. Very important
    virtual void calibrate() = 0; //Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual void read_gps() = 0;
    virtual double get_alt() = 0;
    virtual imu::Vector<3> get_velocity() = 0;
    virtual imu::Vector<2> get_pos() = 0;
    virtual imu::Vector<3> get_displacement() = 0;
    virtual double get_gps_time() = 0;
    virtual int get_fix_qual() = 0;
    virtual String getcsvHeader() = 0;
    virtual String getdataString() = 0;
};


#endif 