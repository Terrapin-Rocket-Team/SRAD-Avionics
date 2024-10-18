//
// To encapsulate a full IMU's functionality, this class combines the sensors of BMI088 and LIS3MDL
// Originally created by ramykaddouri on 9/24/24.
//

#ifndef BMI088andLIS3MDL_H
#define BMI088andLIS3MDL_H
#include <BMI088.h>
#include <Sensors/IMU/IMU.h>


class BMI088andLIS3MDL : public mmfs::IMU {
public:
    //See https://github.com/bolderflight/bmi088-arduino/blob/main/README.md for default addresses
    BMI088andLIS3MDL(const char* name = "BMI088andLIS3MDL", TwoWire &bus = Wire, uint8_t accelAddr = 0x18, uint8_t gyroAddr = 0x68) : accel(bus, accelAddr), gyro(bus, gyroAddr) {
        Sensor::setName(name);
    }
    bool init() override;
    void read() override;

protected:
    Bmi088Accel accel;
    Bmi088Gyro gyro;
};



#endif //BMI088andLIS3MDL_H
