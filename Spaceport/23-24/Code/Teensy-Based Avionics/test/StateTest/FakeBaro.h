#ifndef FAKEBARO_H
#define FAKEBARO_H

#include "Barometer.h"
#include <stdio.h>
class FakeBaro : public Barometer
{
public:
    void feedData(double npress, double ntemp, double nalt)
    {
        press = npress;
        temp = ntemp;
        alt = nalt;
    }
    double get_temp() { return press; }
    double get_pressure() { return temp; }
    double get_rel_alt_m() { return alt; }
    bool initialize() { return true; }

    const char *getcsvHeader()
    {                                                // incl  B- to indicate Barometer data  vvvv Why is this in ft and not m?
        return "B-Pres (hPa),B-Temp (C),B-Alt (m),"; // trailing commas are very important
    }

    char *getdataString()
    { // See State.cpp::setdataString() for comments on what these numbers mean
        // float x3
        const int size = 12 * 3 + 3;
        char *data = new char[size];
        snprintf(data, size, "%.2f,%.2f,%.2f,", get_pressure(), get_temp(), get_rel_alt_m()); // trailing comma
        return data;
    }
    char *getStaticDataString()
    {
        char *data = new char[9];
        snprintf(data, 9, "%s\n", "Testing");
        return data;
    }

    double get_temp_f(){return 0;}
    double get_rel_alt_ft() {return 0;}
    void *get_data(){return nullptr;}
    double get_pressure_atm() {return 0;}

private:
    double press, temp, alt;
};
#endif