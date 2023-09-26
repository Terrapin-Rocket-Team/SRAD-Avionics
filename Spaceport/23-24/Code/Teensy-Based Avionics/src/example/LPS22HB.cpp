//
// Created by kking on 7/24/2023.
//
#include "LPS22HB.h"

LPS22HB::LPS22HB(I2C_HandleTypeDef* i2c_config, uint16_t sensor_address)
{
    this->address = sensor_address;
    this->i2c_config = i2c_config;
};

uint8_t LPS22HB::init()
{
    //Make sure the sensor is powered on and discovered on the I2C bus
    HAL_StatusTypeDef i2c_status = HAL_I2C_IsDeviceReady(i2c_config, (uint16_t)(LPS22HB::default_address<<1), 3, 5);
    if(i2c_status == HAL_BUSY) {
        return 0;
    }

    //Set the ouptut data rate (odr)
    this->set_odr(ODR_75hz);

    //Make sure the odr matches the desired odr
    enum LPS22HB::ODR_CONFIG odr = this->get_odr();
    if(odr != ODR_75hz) {
        return 0;
    }

    //Set FIFO mode to BYPASS
    this->configure_fifo(BYPASS);

    //Enable low pass filter
    //LPS_Configure_LPFP(ODR_9);

    return 1;
}

void LPS22HB::reg_read(uint16_t reg_addr, uint16_t reg_size, uint8_t* data_output)
{
    HAL_I2C_Mem_Read(this->i2c_config, (uint16_t)(this->address<<1), reg_addr, 1, data_output, reg_size, 100);
}

//Configures the output data rate (ODR)
void LPS22HB::set_odr(enum LPS22HB::ODR_CONFIG new_odr)
{
    //Read the existing configuration from the CTRL_REG 1
    uint8_t existingConfig[1];
    this->reg_read(CTRL_REG1, 1, existingConfig);

    //Clear top 4 bits from the existing config
    uint8_t newConfig[1];
    newConfig[0] = existingConfig[0] & 0x0F;

    //Store the new ODR configuration
    newConfig[0] |= (new_odr << 4);

    //Write new ODR to register
    HAL_I2C_Mem_Write(this->i2c_config, (uint16_t)(this->address<<1), CTRL_REG1, 1, newConfig, 1, 100);
}

//Reads the sensors odr
enum LPS22HB::ODR_CONFIG LPS22HB::get_odr()
{
    uint8_t existingConfig[1];
    this->reg_read(CTRL_REG1, 1, existingConfig);
    uint8_t odr_code = (existingConfig[0] >> 4);

    return static_cast<LPS22HB::ODR_CONFIG>(odr_code);
}

//Configures the FIFO for the desired mode
void LPS22HB::configure_fifo(enum LPS22HB::FIFO_CONFIG desiredFifo)
{
    //Only supports bypass mode for now
    if(desiredFifo != BYPASS) {
        return;
    }

    //Read existing FIFO config from sensor
    uint8_t existingConfig[1];
    this->reg_read(FIFO_CTRL, 1, existingConfig);

    //Clear the top 3 bits from the existing config
    uint8_t newConfig[1];
    newConfig[0] = existingConfig[0] & 0x1F;

    //Write new FIFO config to sensor
    HAL_I2C_Mem_Write(this->i2c_config, (uint16_t) (this->address<<1), FIFO_CTRL, 1, newConfig, 1, 100);
}

//Returns the pressure read by the sensor in HPA
double LPS22HB::get_pressure()
{
    double SCALING_FACTOR = 4096.0;

    uint8_t press_out_h[1];
    uint8_t press_out_l[1];
    uint8_t press_out_xl[1];

    this->reg_read(PRESS_OUT_H, 1, press_out_h);
    this->reg_read(PRESS_OUT_L, 1, press_out_l);
    this->reg_read(PRESS_OUT_XL, 1, press_out_xl);

    uint32_t pressure = (press_out_h[0] << 16) + (press_out_l[0] << 8) + press_out_xl[0];

    return pressure / SCALING_FACTOR;
}

//Returns the temperature read by the sensor in C
double LPS22HB::get_temp()
{
    double SCALING_FACTOR = 100.0;
    uint8_t temp_out_h[1];
    uint8_t temp_out_l[1];

    this->reg_read(TEMP_OUT_H, 1, temp_out_h);
    this->reg_read(TEMP_OUT_L, 1, temp_out_l);

    uint32_t temperature = (temp_out_h[0] << 8) + temp_out_l[0];

    return temperature / SCALING_FACTOR;
}

void LPS22HB::calibrate()
{
    double pressure = this->get_pressure();
    this->reference_pressure = pressure;
}

double LPS22HB::get_temp_f()
{
    double tempC = this->get_temp();
    return (tempC * (9/5.0)) + 32;
}

double LPS22HB::get_pressure_atm()
{
    double pressHPA = this->get_pressure();
    return pressHPA * 0.0009869233;
}

//Reference pressure is pressure in Pa at surface
double LPS22HB::get_rel_alt_ft()
{
    double p = this->get_pressure();
    double t = this->get_temp();

    //Hypsometric formula: https://keisan.casio.com/exec/system/1224585971
    double frac_p = (double)this->reference_pressure / p;
    double exponential = pow(frac_p, (double)1.0/5.257);
    double fraction_top = (exponential - 1) * (t + 273.15);
    double alt_m = fraction_top / 0.0065;
    return alt_m * 3.281; //Convert to ft and return
}
