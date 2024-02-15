#ifndef BMP390_H
#define BMP390_H

#include <Adafruit_BMP3XX.h>
#include <Arduino.h>
#include "Barometer.h"

#define SEALEVELPRESSURE_HPA (1013.25)


class BMP390: public Barometer {
private:
    Adafruit_BMP3XX bmp;
    uint8_t SCK_pin;
    uint8_t SDA_pin;
    double groundPressure;
    double pressure;        // hPa
    double temp;            // C
    double altitude;          // m

public:
    BMP390(uint8_t SCK, uint8_t SDA); 
    void initialize(); 
    double get_pressure();
    double get_temp();
    double get_temp_f();
    double get_pressure_atm();
    double get_rel_alt_ft();
    double get_rel_alt_m();
    void * get_data();
    std::vector<String> getcsvHeader();
    String getdataString();
    String getStaticDataString();
};


#endif //BMP390_H
