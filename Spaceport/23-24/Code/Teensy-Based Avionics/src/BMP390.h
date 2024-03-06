#ifndef BMP390_H
#define BMP390_H

#include <Adafruit_BMP3XX.h>
#include <Arduino.h>
#include "Barometer.h"

#define SEALEVELPRESSURE_HPA (1013.25)

class BMP390 : public Barometer
{
private:
    Adafruit_BMP3XX bmp;
    uint8_t SCKPin;
    uint8_t SDAPin;
    double groundPressure;
    double pressure; // hPa
    double temp;     // C
    double altitude; // m

public:
    BMP390(uint8_t SCK, uint8_t SDA);
    double getPressure();
    double getTemp();
    double getTempF();
    double getPressureAtm();
    double getRelAltFt();
    double getRelAltM();
    void *getData();
    bool initialize() override;
    const char *getCsvHeader() override;
    char *getDataString() override;
    char *getStaticDataString() override;
    char const *getName() override;
    void update() override;
};

#endif // BMP390_H
