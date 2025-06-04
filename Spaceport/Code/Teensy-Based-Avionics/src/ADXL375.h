#ifndef ADXL375_H
#define ADXL375_H
#include <Adafruit_ADXL375.h>
#include <Wire.h>
#include <vector>
#include <Sensors/IMU/IMU.h>


#define ADXL375_ADDRESS (0x1D) //alt address pin tied low for our case 



    class Adafruit_ADXL375Wrap : public mmfs::IMU {
        public:

            Adafruit_ADXL375Wrap(): accel(ID, &Wire2)
            {
                 setName("ADXL");
            }

            bool init();
            void read();

        private:
            Adafruit_ADXL375 accel;

            static const unsigned int ID = 2;
            bool initialized;


    };

#endif







