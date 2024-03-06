#ifndef FAKEGPS_H
#define FAKEGPS_H

#include <stdio.h>
#include "GPS.h"

class FakeGPS : public GPS
{
public:
    void feedData(double posX, double posY, double posZ, double h)
    {
        if(initialLatitude == 0 || initialLongitude == 0){
            initialLatitude = posX;
            initialLongitude = posY;
            initAltitude = posZ;
        }
        pos.x() = (posX - initialLatitude) * 111319;
        pos.y() = (posY - initialLongitude) * 111319 * cos(posX * 3.14159 / 180);
        alt = posZ - initAltitude;//hacky way to store initial altitude
        heading = h;
    }
    bool initialize() override { return true; }
    imu::Vector<2> getPos() { return pos; }
    double getAlt(){return alt;}
    imu::Vector<3> getVelocity() {return imu::Vector<3>(0,0,0);}

    double getHeading() override { return heading; }

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
    imu::Vector<3> getOriginPos() { return imu::Vector<3>(0, 0, 0); }
    imu::Vector<3> getDisplace() { return imu::Vector<3>(0, 0, 0); }
    int getFixQual(){return 6;}
    const char *getName() override { return "FakeGPS"; }
    void update() override {}
    char *getTimeOfDay() override { return "00:00:00"; }

private:
    imu::Vector<2> pos;
    double alt, heading;
    double initialLatitude = 0;
    double initialLongitude = 0;
    double initAltitude = 0;
};
#endif