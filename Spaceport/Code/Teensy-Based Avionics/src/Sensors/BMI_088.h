//
// Created by ramykaddouri on 9/24/24.
//

#ifndef BMI088_H
#define BMI088_H
#include <BMI088.h>
#include <Sensors/IMU/IMU.h>

class BMI_088 : public mmfs::IMU {
public:
    BMI_088(TwoWire &bus,uint8_t address) : accel(bus, address), gyro(bus, address) {
    }
    ~BMI_088() override;

    bool init() override;
    void read() override;
    //currently there's no corresponding sensor type
    //for this, should be added to MMFS
protected:
    Bmi088Accel accel;
    Bmi088Gyro gyro;
};



#endif //BMI088_H
