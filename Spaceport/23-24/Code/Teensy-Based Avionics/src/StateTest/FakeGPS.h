#ifndef FAKEGPS_H
#define FAKEGPS_H

#include <stdio.h>
#include "GPS.h"

class FakeGPS : public GPS
{
public:
    void read_gps() {}
    void feedData(double pos_x, double pos_y, double pos_z, double heading)
    {
        pos.x() = pos_x;
        pos.y() = pos_y;
        alt = pos_z;
        this->heading = heading;
    }
    bool initialize() override { return true; }
    imu::Vector<2> get_pos() { return pos; }
    double get_alt(){return alt;}
    imu::Vector<3> get_velocity() {return imu::Vector<3>(0,0,0);}

    double get_heading() override { return heading; }

    const char *getCsvHeader() override
    {                                                                                                                                                // incl G- for GPS
        return ","; //dont care abt GPS data for testing
    }
    char *getDataString() override
    {
        char *data = new char[2];
        snprintf(data, 2, "%s",",");
        return data;
    }
    char *getStaticDataString() override
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    void *getData() override { return nullptr; }
    imu::Vector<3> get_origin_pos() { return imu::Vector<3>(0, 0, 0); }
    imu::Vector<3> get_displace() { return imu::Vector<3>(0, 0, 0); }
    double get_gps_time(){return 0;}
    int get_fix_qual(){return 0;}
    const char *getName() override { return "FakeGPS"; }
    void update() override {}
    char *get_time_of_day() override { return "00:00:00"; }

private:
    imu::Vector<2> pos;
    double alt, heading;
};
#endif