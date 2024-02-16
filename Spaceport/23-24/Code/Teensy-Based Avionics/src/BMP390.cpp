#include "BMP390.h"

/*
Construtor for the BMP390 class, pass in the pin numbers for each of the I2C pins on the MCU
*/
BMP390::BMP390(uint8_t SCK, uint8_t SDA)
{

    SCK_pin = SCK;
    SDA_pin = SDA;
}

bool BMP390::initialize()
{

    if (!bmp.begin_I2C())
    { // hardware I2C mode, can pass in address & alt Wire
        // Serial.println("Could not find a valid BMP390 sensor, check wiring!");
        return false;
    }

    delay(1000);

    // Set up oversampling and filter initialization
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);

    double startPressure = 0;
    for (int i = 0; i < 10; i++)
    {
        bmp.readPressure();
        delay(25);
    }
    for (int i = 0; i < 100; i++)
    {
        startPressure += bmp.readPressure();
        delay(25);
    }
    groundPressure = (startPressure / 100) / 100.0; // hPa
    return true;
}

double BMP390::get_pressure()
{
    pressure = bmp.readPressure() / 100.0; // hPa
    return pressure;
}

double BMP390::get_temp()
{
    temp = bmp.readTemperature(); // C
    return temp;
}

double BMP390::get_temp_f()
{
    return (get_temp() * 9.0 / 5.0) + 32.0;
}

double BMP390::get_pressure_atm()
{
    return get_pressure() / SEALEVELPRESSURE_HPA;
}

double BMP390::get_rel_alt_m()
{
    altitude = bmp.readAltitude(groundPressure); // meters
    return altitude;
}

double BMP390::get_rel_alt_ft()
{
    return get_rel_alt_m() * 3.28084;
}

void *BMP390::get_data()
{
    return (void *)&altitude;
}

char *BMP390::getcsvHeader()
{ // incl  B- to indicate Barometer data  vvvv Why is this in ft and not m?
    return "B-Pres (hPa),B-Temp (C),B-Alt (ft),"; // trailing commas are very important
}

char *BMP390::getdataString()
{ // See State.cpp::setdataString() for comments on what these numbers mean
    // float x3
    const int size = 12 * 3 + 3;
    char data[size];
    snprintf(data, size, "%.2f,%.2f,%.2f,", get_pressure(), get_temp(), get_rel_alt_ft());//trailing comma
    return data;
}

char *BMP390::getStaticDataString()
{ // See State.cpp::setdataString() for comments on what these numbers mean
    const int size = 25 + 12 * 1;
    char data[size];
    snprintf(data, size, "Ground Pressure (hPa): %.2f\n", groundPressure);
    return data;
}