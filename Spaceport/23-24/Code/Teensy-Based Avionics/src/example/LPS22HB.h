//
// Created by kking on 7/24/2023.
//

#ifndef LPS22HB_H
#define LPS22HB_H
#include "stm32f1xx_hal.h"
#include "math.h"
#include "Barometer.h"

class LPS22HB: public Barometer {
private:
    uint8_t address;
    I2C_HandleTypeDef* i2c_config;

public:
    //Register map for the sensor
    enum REG_MAP {
        CTRL_REG1 = 0x10,
        CTRL_REG2 = 0x11,
        CTRL_REG3 = 0x12,
        FIFO_CTRL = 0x14,
        REF_P_XL = 0x15,
        REF_P_L = 0x16,
        REF_P_H = 0x17,
        STATUS_REG = 0x27,
        PRESS_OUT_XL = 0x28,
        PRESS_OUT_L = 0x29,
        PRESS_OUT_H = 0x2A,
        TEMP_OUT_L = 0x2B,
        TEMP_OUT_H = 0x2C,
        LPFP_RES = 0x33
    };

    //Output Data Rate Options
    enum ODR_CONFIG {
        ODR_off  = 0,
        ODR_1hz  = 0x1,
        ODR_10hz = 0x2,
        ODR_25hz = 0x3,
        ODR_50hz = 0x4,
        ODR_75hz = 0x5
    };

    //FIFO modes
    enum FIFO_CONFIG {
        BYPASS = 0x0
    };

    //Bandwidth after applying low pass filter
    enum LPFP_BANDWIDTH {
        ODR_2 = 0x0, //ODR/2
        ODR_9 = 0x2, //ODR/9
        ODR_20 = 0x3 //ODR/20
    };

    static const uint8_t default_address = 0x5C; //Sensors default address
    double reference_pressure;

    LPS22HB(I2C_HandleTypeDef* i2c_config, uint16_t sensor_address);
    uint8_t init();
    void reg_read(uint16_t reg_addr, uint16_t reg_size, uint8_t* data_output);
    void set_odr(enum LPS22HB::ODR_CONFIG new_odr);
    enum LPS22HB::ODR_CONFIG get_odr();
    void configure_fifo(enum LPS22HB::FIFO_CONFIG desiredFifo);
    void configure_lpfp(enum LPS22HB::LPFP_BANDWIDTH bandwidth);
    void calibrate();
    double get_pressure();
    double get_temp();
    double get_temp_f();
    double get_pressure_atm();
    double get_rel_alt_ft();
};


#endif //MULTIMISSIONLIBRARY_LPS22HB_H