#ifndef ADXL375_H
#define ADXL375_H
#include <Adafruit_ADXL375.h>
#include <Wire.h>
#include <vector>
#include <Sensors/IMU/IMU.h>

    class ADXL375 : public mmfs::IMU {
        public:

            ADXL375(const char *name = "ADXL375", TwoWire &bus = Wire2, uint8_t address = 0x1D);
            
            bool init() override;
            bool read() override;

        private:
            Adafruit_ADXL375 accel;
            uint8_t addr; // Default I2C address for ADXL375
          


    };

#endif







