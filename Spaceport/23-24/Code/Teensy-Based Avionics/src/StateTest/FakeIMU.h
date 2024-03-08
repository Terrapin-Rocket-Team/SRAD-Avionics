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
    bool initialize() override { return true; }
    imu::Quaternion getOrientation() override { return ori; }
    imu::Vector<3> getAcceleration() override { return acc; }

    const char *getCsvHeader() override
    {                                                                                     // incl I- for IMU
        return "I-AX (m/s/s),I-AY (m/s/s),I-AZ (m/s/s),I-QUATX,I-QUATY,I-QUATZ,I-QUATW,"; // trailing comma
    }

    char *getDataString() override
    {
        const int size = 12 * 7 + 10;
        char *data = new char[size];
        snprintf(data, size, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", acc.x(), acc.y(), acc.z(), ori.x(), ori.y(), ori.z(), ori.w()); // trailing comma"
        return data;
    }

    char *getStaticDataString() override
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    imu::Vector<3> getOrientationEuler() override { return imu::Vector<3>(0, 0, 0); }
    imu::Vector<3> getMagnetometer() override { return imu::Vector<3>(0, 0, 0); }
    void *getData() override { return nullptr; }
    const char *getName() override { return "FakeIMU"; }
    void update() override {}

private:
    imu::Quaternion ori = imu::Quaternion(1, 0, 0, 0);
    imu::Vector<3> acc = imu::Vector<3>(0, 0, 0);
};

#endif