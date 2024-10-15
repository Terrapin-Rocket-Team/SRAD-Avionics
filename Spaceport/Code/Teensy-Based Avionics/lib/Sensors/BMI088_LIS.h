//
// Created by ramykaddouri on 9/24/24.
//

#ifndef BMI088_LIS_H
#define BMI088_LIS_H
#include "BMI088_LIS.h"
#include <Sensors/IMU/IMU.h>


class BMI088 : public mmfs::IMU {
public:
    //See https://github.com/bolderflight/bmi088-arduino/blob/main/README.md for default addresses
    BMI088(const char* name = "BMI088", TwoWire &bus = Wire, uint8_t accelAddr = 0x18, uint8_t gyroAddr = 0x68) : accel(bus, accelAddr), gyro(bus, gyroAddr) {
        Sensor::setName(name);
    }
    bool init() override;
    void read() override;

protected:
    Bmi088Accel accel;
    Bmi088Gyro gyro;
    LISMag mag;
};



#endif //BMI088_H
