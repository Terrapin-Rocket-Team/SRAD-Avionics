#include "ADXL375.h"

using namespace mmfs;

ADXL375::ADXL375(const char *name, TwoWire &bus, uint8_t address) : Accel(name), accel(0, &bus), addr(address)
{
}

bool ADXL375::init()
{
    if (this->accel.begin(addr))
    {
        accel.setTrimOffsets(-3, -3, -1); // Z should be '20' at 1g (49mg per bit)
        return true;
    }
    return false;
}

bool ADXL375::read()
{
    sensors_event_t event;
    if (this->accel.getEvent(&event))
    {
        acc = Vector<3>((double)event.acceleration.x, (double)event.acceleration.y, (double)event.acceleration.z);
        return true;
    }
    else
        return false;
}
