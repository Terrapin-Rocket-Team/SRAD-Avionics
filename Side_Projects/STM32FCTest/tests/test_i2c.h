#ifndef TEST_I2C_H
#define TEST_I2C_H

#include <Arduino.h>
#include <Wire.h>

namespace I2CTest {
    void setup();
    void run();
    void scanBus();
}

#endif // TEST_I2C_H
