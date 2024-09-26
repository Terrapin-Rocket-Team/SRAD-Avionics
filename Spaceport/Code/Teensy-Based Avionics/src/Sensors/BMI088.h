//
// Created by ramykaddouri on 9/24/24.
//

#ifndef BMI088_H
#define BMI088_H
#include <BMI088.h>
#include <Sensors/IMU/IMU.h>

class BMI088 : public mmfs::IMU {
public:
    BMI088(TwoWire &bus,uint8_t address) : accel(bus, address), gyro(bus, address) {
        Sensor::setName("BMI088");
    }
    ~BMI088() override;

    bool init() override;
    void read() override;

protected:
    Bmi088Accel accel;
    Bmi088Gyro gyro;
};



#endif //BMI088_H
