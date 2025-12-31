#ifndef H3LIS331DL_H
#define H3LIS331DL_H
#include <Wire.h>
#include <Sensors/Accel/Accel.h>
#include <SparkFun_LIS331.h>

using namespace astra;
class H3LIS331DL : public Accel
{
public:
    H3LIS331DL(const char *name = "H3LIS331DL", TwoWire &bus = Wire, uint8_t address = 0x19);

    bool init() override;
    bool read() override;

private:
    LIS331 accel;
    uint8_t addr; // Default I2C address for ADXL375
};

#endif
