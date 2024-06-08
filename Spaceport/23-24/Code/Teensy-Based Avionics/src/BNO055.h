#include <Adafruit_BNO055.h>
#include <Arduino.h>
#include "IMU.h"
#include "RecordData.h"

class BNO055 : public IMU
{
private:
    Adafruit_BNO055 bno;
    uint8_t SCKPin;
    uint8_t SDAPin;
    imu::Vector<3> accelerationVec;
    imu::Vector<3> orientationEuler;
    imu::Quaternion orientation;
    imu::Vector<3> magnetometer;
    imu::Vector<3> initialMagField;
    imu::Vector<3> prevReadings[20];

    void setCalibrationFromFile();

public:
    void calibrateBno();
    imu::Quaternion getOrientation() override;
    // gives linearAcceleration in m/s/s, which excludes gravity
    imu::Vector<3> getAcceleration() override;
    imu::Vector<3> getOrientationEuler() override;
    // values in uT, micro Teslas
    imu::Vector<3> getMagnetometer() override;
    bool initialize() override;
    const char *getCsvHeader() override;
    char *getDataString() override;
    char *getStaticDataString() override;
    char const *getName() override;
    void update() override;
    void setBiasCorrectionMode(bool mode) override;
    void recordCalibrationValues();
    void getSensorOffsets(adafruit_bno055_offsets_t &offsets);
    void getCalibrationStatus(uint8_t &system, uint8_t &gyro, uint8_t &accel, uint8_t &mag);
};

// gives it in rotations about the x, y, z (yaw, pitch, roll) axes
imu::Vector<3> convertToEuler(const imu::Quaternion &orientation);
