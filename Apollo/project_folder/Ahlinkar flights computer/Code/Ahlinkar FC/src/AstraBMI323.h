#ifndef AHLINKAR_ASTRA_BMI323_H
#define AHLINKAR_ASTRA_BMI323_H

#include <Arduino.h>
#include <Wire.h>
#include <Sensors/IMU/IMU6DoF.h>

extern "C"
{
#include <bmi323.h>
}

namespace astra
{
    class BMI323 : public IMU6DoF
    {
    public:
        BMI323(const char *name = "BMI323", TwoWire *bus = &Wire, uint8_t i2cAddress = BMI3_ADDR_I2C_PRIM);
        BMI323(TwoWire *bus, uint8_t i2cAddress = BMI3_ADDR_I2C_PRIM);

        int init() override;
        int read() override;

    private:
        static BMI3_INTF_RET_TYPE i2cRead(uint8_t regAddr, uint8_t *data, uint32_t len, void *intfPtr);
        static BMI3_INTF_RET_TYPE i2cWrite(uint8_t regAddr, const uint8_t *data, uint32_t len, void *intfPtr);
        static void delayUs(uint32_t periodUs, void *intfPtr);

        static constexpr float ACC_RANGE_G = 16.0f;
        static constexpr float GYRO_RANGE_DPS = 2000.0f;

        TwoWire *wire;
        uint8_t addr;
        struct bmi3_dev dev;
        struct bmi3_sensor_data sensorData[2];
    };
}

#endif
