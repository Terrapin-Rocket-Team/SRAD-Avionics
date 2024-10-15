#include "BMP390.h"

namespace mmfs
{

    BMP390::BMP390(const char *name) : bmp()
    {
        setName(name);
    }

    bool BMP390::init()
    {
        if (!bmp.begin_I2C())
        { // hardware I2C mode, can pass in address & alt Wire
            // Serial.println("Could not find a valid BMP390 sensor, check wiring!");
            return initialized = false;
        }

        // delay(1000);

        // Set up oversampling and filter initialization
        int good = 0;
        good += bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
        good += bmp.setPressureOversampling(BMP3_OVERSAMPLING_32X);
        good += bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
        good += bmp.setOutputDataRate(BMP3_ODR_50_HZ);

        if (good != 4) // If any of the above failed
            return initialized = false;
    
        return initialized = true;
    }

    void BMP390::read()
    {
        pressure = bmp.readPressure() / 100.0;       // hPa
        temp = bmp.readTemperature();                // C
    }
}