#ifndef ADXL375_H
#define ADXL375_H
#include <Adafruit_ADXL375.h>
#include <Wire.h>
#include <vector>
#include <Sensors/IMU/IMU.h>


#define ADDRESS (0x53) //alt address pin tied low for our case 



    class Adafruit_ADXL375Wrap : public mmfs::IMU {
        public:

            Adafruit_ADXL375Wrap(): accel(ID++) //using default wire bus
            {}

            bool init();
            void read();

        private:
            Adafruit_ADXL375 accel;

            static signed int ID;
            bool initialized;


    };

#endif







