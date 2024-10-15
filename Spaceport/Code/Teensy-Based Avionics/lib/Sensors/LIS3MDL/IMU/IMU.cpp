#include "IMU.h"

namespace mmfs
{

    Quaternion IMU::getOrientationGlobal()
    {
        return orientationGlobal;
    }

    Vector<3> IMU::getAccelerationGlobal()
    {
        return accelerationGlobal;
    }

    Quaternion IMU::getOrientationLocal()
    {
        return orientationLocal;
    }

    Vector<3> IMU::getAccelerationLocal()
    {
        return accelerationLocal;
    }

    Vector<3> IMU::getMagnetometerReading()
    {
        return measuredMag;
    }

    Vector<3> IMU::getGyroReading()
    {
        return measuredGyro;
    }

    Vector<3> IMU::getAccReading()
    {
        return measuredAcc;
    }

    const char *IMU::getCsvHeader() const
    {                                                                                                          // incl I- for IMU
        return "I-AX (m/s/s),I-AY (m/s/s),I-AZ (m/s/s),I-QUATX,I-QUATY,I-QUATZ,I-QUATW,"; // trailing comma
    }

    const char *IMU::getDataString() const
    {
        sprintf(data, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", measuredAcc.x(), measuredAcc.y(), measuredAcc.z(), orientationGlobal.x(), orientationGlobal.y(), orientationGlobal.z(), orientationGlobal.w()); // trailing comma"
        return data;
    }

    const char *IMU::getStaticDataString() const
    {
        sprintf(data, "Initial Magnetic Field (uT): %.2f,%.2f,%.2f\n", initialMagField.x(), initialMagField.y(), initialMagField.z());
        return data;
    }
    void IMU::update()
    {
        read();
    }

    bool IMU::begin(bool useBiasCorrection)
    {
        biasCorrectionMode = useBiasCorrection;
        return init();
    }
}