#include "BMP390.h"

/*
Construtor for the BMP390 class, pass in the pin numbers for each of the I2C pins
*/
BMP390::BMP390(uint8_t SCK, uint8_t SDA) {

    SCK_pin = SCK;
    SDA_pin = SDA; 

}

void BMP390::calibrate() {

    Serial.begin(115200);
    while (!Serial);
    Serial.println("BMP390 on.");

    if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
    //if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
    //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
        Serial.println("Could not find a valid BMP390 sensor, check wiring!");
        while (1);
    }

    // Set up oversampling and filter initialization
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);

    double startPressure = 0;
    for (int i = 0; i < 10; i++)
    {
        startPressure += bmp.readPressure();
        delay(100);
    }
    groundPressure = (startPressure/10)/100.0; // hPa
}

double BMP390::get_pressure() {
    pressure = bmp.readPressure() / 100.0; // hPa
    return pressure;
}

double BMP390::get_temp() {
    temp = bmp.readTemperature(); // C
    return temp;
}

double BMP390::get_temp_f() {
    return (get_temp() * 9.0 / 5.0) + 32.0;
}

double BMP390::get_pressure_atm() {
    return get_pressure() / SEALEVELPRESSURE_HPA;
}

double BMP390::get_rel_alt_m() {
    altitude = bmp.readAltitude(groundPressure); // meters
    return altitude;
}

double BMP390::get_rel_alt_ft() {
    return get_rel_alt_m() * 3.28084;
}

void * BMP390::get_data() {
    return (void *) &altitude;
}

String BMP390::getcsvHeader() {
    return "Pressure (hPa),Temperature (C),Altitude (m)";
}

String BMP390::getdataString() {
    return String(pressure) + "," + String(temp) + "," + String(altitude);
}

String BMP390::getStaticDataString() {
    return "Ground Pressure (hPa):" + String(groundPressure) + "\n";
}