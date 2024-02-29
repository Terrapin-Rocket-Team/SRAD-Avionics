#include <Arduino.h>
#include "State.h"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "RFM69HCW.h"
#include <RecordData.h>

BNO055 bno(13, 12);         // I2C Address 0x29
BMP390 bmp(13, 12);         // I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); // I2C Address 0x42
DS3231 rtc();               // I2C Address 0x68
APRSConfig config = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
RadioSettings settings = {433.775, true, false, &hardware_spi, 10, 31, 32};
RFM69HCW radio = {settings, config};
State computer;
uint32_t dataTimer = millis();
uint32_t radioTimer = millis();

#define BUZZER 33
#define BMP_ADDR_PIN 36

void setup()
{
    // Setup BMP to use defualt address
    pinMode(BMP_ADDR_PIN, OUTPUT);
    digitalWrite(BMP_ADDR_PIN, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(BUZZER, OUTPUT);

    Serial.begin(9600);
    while (!Serial)
        ;

    computer.setBaro(&bmp);
    // computer.addGPS(&gps);
    // computer.addRTC(&rtc);
    computer.setIMU(&bno);
    computer.setRadio(&radio);

    computer.init();
    setupPSRAM(computer.getcsvHeader());
    bool sdSuccess = setupSDCard(computer.getcsvHeader());

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    if (sdSuccess)
    {
        // Serial.println("SD Card initialized");
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
    }
    else
    {
        // Serial.println("SD Card failed to initialize");
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(200);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
    }
}

void loop()
{

    if (millis() - dataTimer >= 50)
    {
        computer.updateState();
        Serial.println(computer.getStateString());
        recordData(computer.getdataString(), computer.getStageNum());
        dataTimer = millis();
    }
    if (millis() - radioTimer >= 2000)
    {
        computer.transmit();
        radioTimer = millis();
    }
}