#ifndef TEENSY_FC_TEST_I2C_H
#define TEENSY_FC_TEST_I2C_H

#include <Arduino.h>
#include <Wire.h>

namespace I2CTest {
void setup();
void run();
void scanBus();
}  // namespace I2CTest

#endif  // TEENSY_FC_TEST_I2C_H
