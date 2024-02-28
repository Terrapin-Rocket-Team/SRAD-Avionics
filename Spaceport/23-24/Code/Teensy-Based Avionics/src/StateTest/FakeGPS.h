#ifndef FAKEGPS_H
#define FAKEGPS_H

#include <stdio.h>
#include "GPS.h"

class FakeGPS : public GPS
{
public:
    void read_gps() {}
    void feedData(double pos_x, double pos_y, double pos_z)
    {
        pos.x() = pos_x;
        pos.y() = pos_y;
        alt = pos_z;
    }
    bool initialize() { return false; }
    imu::Vector<2> get_pos() { return pos; }
    double get_alt(){return alt;}
    imu::Vector<3> get_velocity() {return imu::Vector<3>(0,0,0);}

    const char *getCsvHeader()
    {                                                                                                                                                // incl G- for GPS
        return ","; //dont care abt GPS data for testing
    }
    char *getDataString()
    {
        char *data = new char[2];
        snprintf(data, 2, "%s",",");
        return data;
    }
    char *getStaticDataString()
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    void * getData(){return nullptr;}
    imu::Vector<3> get_origin_pos() { return imu::Vector<3>(0, 0, 0); }
    imu::Vector<3> get_displace() { return imu::Vector<3>(0, 0, 0); }
    double get_gps_time(){return 0;}
    int get_fix_qual(){return 0;}
    const char *getName() { return "FakeGPS"; }
    void update() {}
private:
    imu::Vector<2> pos;
    double alt;
};
#endif