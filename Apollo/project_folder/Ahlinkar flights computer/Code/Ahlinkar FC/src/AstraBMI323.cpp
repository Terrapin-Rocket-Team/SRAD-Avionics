#include "AstraBMI323.h"

namespace astra
{
    BMI323::BMI323(const char *name, TwoWire *bus, uint8_t i2cAddress)
        : IMU6DoF(name), wire(bus), addr(i2cAddress), dev{}, sensorData{}
    {
    }

    BMI323::BMI323(TwoWire *bus, uint8_t i2cAddress)
        : BMI323("BMI323", bus, i2cAddress)
    {
    }

    int BMI323::init()
    {
        if (wire == nullptr)
        {
            return BMI3_E_NULL_PTR;
        }

        wire->begin();

        dev.read = i2cRead;
        dev.write = i2cWrite;
        dev.delay_us = delayUs;
        dev.intf = BMI3_I2C_INTF;
        dev.intf_ptr = this;
        dev.read_write_len = 32;

        int8_t rslt = bmi323_init(&dev);
        if (rslt != BMI323_OK)
        {
            return rslt;
        }

        bmi3_sens_config cfg[2] = {};
        cfg[0].type = BMI323_ACCEL;
        cfg[1].type = BMI323_GYRO;

        rslt = bmi323_get_sensor_config(cfg, 2, &dev);
        if (rslt != BMI323_OK)
        {
            return rslt;
        }

        cfg[0].cfg.acc.odr = BMI3_ACC_ODR_200HZ;
        cfg[0].cfg.acc.range = BMI3_ACC_RANGE_16G;
        cfg[0].cfg.acc.bwp = BMI3_ACC_BW_ODR_QUARTER;
        cfg[0].cfg.acc.avg_num = BMI3_ACC_AVG8;
        cfg[0].cfg.acc.acc_mode = BMI3_ACC_MODE_NORMAL;

        cfg[1].cfg.gyr.odr = BMI3_GYR_ODR_200HZ;
        cfg[1].cfg.gyr.range = BMI3_GYR_RANGE_2000DPS;
        cfg[1].cfg.gyr.bwp = BMI3_GYR_BW_ODR_HALF;
        cfg[1].cfg.gyr.avg_num = BMI3_GYR_AVG1;
        cfg[1].cfg.gyr.gyr_mode = BMI3_GYR_MODE_NORMAL;

        rslt = bmi323_set_sensor_config(cfg, 2, &dev);
        if (rslt != BMI323_OK)
        {
            return rslt;
        }

        sensorData[0].type = BMI323_ACCEL;
        sensorData[1].type = BMI323_GYRO;

        return 0;
    }

    int BMI323::read()
    {
        int8_t rslt = bmi323_get_sensor_data(sensorData, 2, &dev);
        if (rslt != BMI323_OK)
        {
            return rslt;
        }

        constexpr float kHalfScale = 32768.0f;
        constexpr float kGravity = 9.80665f;
        const float accelScale = (ACC_RANGE_G / kHalfScale) * kGravity;
        const float gyroScale = (GYRO_RANGE_DPS / kHalfScale) * DEG_TO_RAD;

        acc = Vector<3>(
            sensorData[0].sens_data.acc.x * accelScale,
            sensorData[0].sens_data.acc.y * accelScale,
            sensorData[0].sens_data.acc.z * accelScale);

        angVel = Vector<3>(
            sensorData[1].sens_data.gyr.x * gyroScale,
            sensorData[1].sens_data.gyr.y * gyroScale,
            sensorData[1].sens_data.gyr.z * gyroScale);

        return 0;
    }

    BMI3_INTF_RET_TYPE BMI323::i2cRead(uint8_t regAddr, uint8_t *data, uint32_t len, void *intfPtr)
    {
        BMI323 *self = static_cast<BMI323 *>(intfPtr);
        if (self == nullptr || self->wire == nullptr || data == nullptr)
        {
            return BMI3_E_NULL_PTR;
        }

        uint32_t offset = 0;
        while (offset < len)
        {
            const uint8_t chunk = static_cast<uint8_t>(((len - offset) < 28U) ? (len - offset) : 28U);
            self->wire->beginTransmission(self->addr);
            self->wire->write(static_cast<uint8_t>(regAddr + offset));
            if (self->wire->endTransmission(false) != 0)
            {
                return BMI3_E_COM_FAIL;
            }

            uint8_t received = static_cast<uint8_t>(
                self->wire->requestFrom(static_cast<int>(self->addr), static_cast<int>(chunk)));
            if (received != chunk)
            {
                return BMI3_E_COM_FAIL;
            }

            for (uint8_t i = 0; i < chunk; i++)
            {
                data[offset + i] = self->wire->read();
            }
            offset += chunk;
        }

        return BMI3_INTF_RET_SUCCESS;
    }

    BMI3_INTF_RET_TYPE BMI323::i2cWrite(uint8_t regAddr, const uint8_t *data, uint32_t len, void *intfPtr)
    {
        BMI323 *self = static_cast<BMI323 *>(intfPtr);
        if (self == nullptr || self->wire == nullptr || data == nullptr)
        {
            return BMI3_E_NULL_PTR;
        }

        uint32_t offset = 0;
        while (offset < len)
        {
            const uint8_t chunk = static_cast<uint8_t>(((len - offset) < 27U) ? (len - offset) : 27U);
            self->wire->beginTransmission(self->addr);
            self->wire->write(static_cast<uint8_t>(regAddr + offset));
            for (uint8_t i = 0; i < chunk; i++)
            {
                self->wire->write(data[offset + i]);
            }

            if (self->wire->endTransmission() != 0)
            {
                return BMI3_E_COM_FAIL;
            }
            offset += chunk;
        }

        return BMI3_INTF_RET_SUCCESS;
    }

    void BMI323::delayUs(uint32_t periodUs, void *intfPtr)
    {
        (void)intfPtr;
        delayMicroseconds(periodUs);
    }
}
