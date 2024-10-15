#include "Barometer.h"
#include <cmath>
namespace mmfs
{

    double Barometer::getPressure() const
    {
        return pressure;
    }

    double Barometer::getTemp() const
    {
        return temp;
    }

    double Barometer::getTempF() const
    {
        return (temp * 9.0 / 5.0) + 32.0;
    }

    double Barometer::getPressureAtm() const
    {
        return pressure / MEAN_SEA_LEVEL_PRESSURE_HPA;
    }

    double Barometer::getAGLAltM() const
    {
        return altitudeAGL;
    }

    double Barometer::getAGLAltFt() const
    {
        return altitudeAGL * 3.28084;
    }

    double Barometer::getASLAltM() const
    {
        return altitudeASL;
    }

    double Barometer::getASLAltFt() const
    {
        return altitudeASL * 3.28084;
    }
    const char *Barometer::getCsvHeader() const
    {                                                                  // incl  B- to indicate Barometer data  vvvv Why is this in ft and not m?
        return "B-Pres (hPa),B-Temp (C),B-AltASL (ft),B-AltAGL (ft),"; // trailing commas are very important
    }

    const char *Barometer::getDataString() const
    {
        sprintf(data, "%.2f,%.2f,%.2f,%.2f,", pressure, temp, getASLAltFt(), getAGLAltFt()); // trailing comma
        return data;
    }

    const char *Barometer::getStaticDataString() const
    {
        sprintf(staticData, "Ground Pressure (hPa): %.2f\n", groundPressure);
        return staticData;
    }

    void Barometer::update()
    {
        read();
        altitudeASL = calcAltitude(pressure);
        if (biasCorrectionMode)
        {
            pressureBuffer.push(pressure);

            double sum = 0;
            int valsToCount = std::min(pressureBuffer.getCount(), CIRC_BUFFER_LENGTH - CIRC_BUFFER_IGNORE);
            for (int i = 0; i < valsToCount; i++)
            {
                sum += pressureBuffer[i];
            }
            groundPressure = sum / valsToCount / 1.0;
            groundAltitude = calcAltitude(groundPressure);
        }

        altitudeAGL = altitudeASL - groundAltitude;
    }

    bool Barometer::begin(bool useBiasCorrection)
    {
        pressure = 0;
        temp = 0;
        altitudeASL = 0;
        altitudeAGL = 0;
        groundPressure = 0;
        pressureBuffer.clear();
        biasCorrectionMode = useBiasCorrection;
        if (init())
        {
            if (!biasCorrectionMode)
            {
                double startPressure = 0;

                for (int i = 0; i < 100; i++)
                {
                    read();
                    startPressure += pressure;
#ifndef PIO_UNIT_TESTING // Don't delay in unit tests
                    delay(25);
#endif
                }
                groundPressure = (startPressure / 100.0); // hPa
                groundAltitude = calcAltitude(groundPressure);
                printf("Ground Pressure: %.2f hPa\n", groundPressure);
                printf("Ground Altitude: %.2f m\n", groundAltitude);
                altitudeASL = groundAltitude;
            }
            return true;
        }
        return false;
    }

    double Barometer::calcAltitude(double pressure)
    {
        // Equation from NOAA, but for meters: https://www.weather.gov/media/epz/wxcalc/pressureAltitude.pdf
        return 44307.69 * (1.0 - pow(pressure / MEAN_SEA_LEVEL_PRESSURE_HPA, 0.190284));
    }
}