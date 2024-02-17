#ifndef FAKEIMU_H
#define FAKEIMU_H

#include <stdio.h>
#include "IMU.h"

class FakeIMU : public IMU
{

public:
    void feedData(double ax, double ay, double az, double qx, double qy, double qz, double qw)
    {
        acc.x() = ax;
        acc.y() = ay;
        acc.z() = az;
        ori.x() = qx;
        ori.y() = qy;
        ori.z() = qz;
        ori.w() = qw;
    }
    bool initialize() { return true; }
    imu::Quaternion get_orientation() { return ori; }
    imu::Vector<3> get_acceleration() { return acc; }

    const char *getcsvHeader()
    {                                                                                     // incl I- for IMU
        return "I-AX (m/s/s),I-AY (m/s/s),I-AZ (m/s/s),I-QUATX,I-QUATY,I-QUATZ,I-QUATW,"; // trailing comma
    }

    char *getdataString()
    {
        const int size = 12 * 7 + 10;
        char *data = new char[size];
        snprintf(data, size, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", acc.x(), acc.y(), acc.z(), ori.x(), ori.y(), ori.z(), ori.w()); // trailing comma"
        return data;
    }

    char *getStaticDataString()
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    imu::Vector<3> get_orientation_euler() { return imu::Vector<3>(0, 0, 0); }
    imu::Vector<3> get_magnetometer() { return imu::Vector<3>(0, 0, 0); }
    void *get_data() { return nullptr; }

private:
    imu::Quaternion ori;
    imu::Vector<3> acc;
};

#endif