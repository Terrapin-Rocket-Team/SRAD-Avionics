#ifndef FAKEBARO_H
#define FAKEBARO_H

#include "Barometer.h"
#include <stdio.h>
class FakeBaro : public Barometer
{
public:
    void feedData(double nalt, double ntemp, double npress)
    {
        press = npress;
        temp = ntemp;
        alt = nalt;
    }
    double getTemp() override { return press; }
    double getPressure() override { return temp; }
    double getRelAltM() override { return alt; }
    bool initialize() override { return initialized = true; }

    const char *getCsvHeader() override
    {                                                // incl  B- to indicate Barometer data
        return "B-Pres (hPa),B-Temp (C),B-Alt (m),"; // trailing commas are very important
    }

    char *getDataString() override
    { // See AvionicsState.cpp::setDataString() for comments on what these numbers mean
        // float x3
        const int size = 12 * 3 + 3;
        char *data = new char[size];
        snprintf(data, size, "%.2f,%.2f,%.2f,", getPressure(), getTemp(), getRelAltM()); // trailing comma
        return data;
    }
    char *getStaticDataString() override
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    double getTempF() override { return 0; }
    double getRelAltFt() override { return alt *3.29; }
    double getPressureAtm() override { return 0; }
    const char *getName() override { return "FakeBaro"; }
    void update() override {}

private:
    double press = 0, temp = 0, alt = 0;
};
#endif