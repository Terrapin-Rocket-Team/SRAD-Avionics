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
    void *getData() {return &alt; }
    double getTemp() { return press; }
    double get_pressure() { return temp; }
    double getRelAltM() { return alt; }
    bool initialize() { return true; }

    const char *getCsvHeader()
    {                                                // incl  B- to indicate Barometer data
        return "B-Pres (hPa),B-Temp (C),B-Alt (m),"; // trailing commas are very important
    }

    char *getDataString()
    { // See State.cpp::setDataString() for comments on what these numbers mean
        // float x3
        const int size = 12 * 3 + 3;
        char *data = new char[size];
        snprintf(data, size, "%.2f,%.2f,%.2f,", get_pressure(), getTemp(), getRelAltM()); // trailing comma
        return data;
    }
    char *getStaticDataString()
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    double getTempF(){return 0;}
    double getRelAltFt() {return 0;}
    double getPressureAtm() {return 0;}
    const char *getName() { return "FakeBaro"; }
    void update() {}
private:
    double press, temp, alt;
};
#endif