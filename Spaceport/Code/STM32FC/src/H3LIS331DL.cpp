#include "H3LIS331DL.h"

using namespace astra;

// Wire is the only supported bus currently
H3LIS331DL::H3LIS331DL(const char *name, TwoWire &bus, uint8_t address) : Accel(name), addr(address)
{
}

bool H3LIS331DL::init()
{
    accel.setI2CAddr(addr);
    accel.begin(LIS331::USE_I2C);
    accel.setFullScale(LIS331::HIGH_RANGE);
    int16_t x = 0, y = 0, z = 0;
    accel.readAxes(x, y, z);
    return !(x == 0 && 0 == y && z == 0);
}

bool H3LIS331DL::read()
{
    int16_t x, y, z;
    accel.readAxes(x, y, z);
    acc = Vector<3>(accel.convertToG(LIS331::HIGH_RANGE, x), accel.convertToG(LIS331::HIGH_RANGE, y), accel.convertToG(LIS331::HIGH_RANGE, z)) * 9.81;
    return true;
}
