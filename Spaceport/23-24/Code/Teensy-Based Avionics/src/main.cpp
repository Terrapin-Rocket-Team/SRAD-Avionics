#include <Arduino.h>
#include "State.h"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "RecordData.h"

BNO055 bno(13, 12);         // I2C Address 0x29
BMP390 bmp(13, 12);         // I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); // I2C Address 0x42
DS3231 rtc();               // I2C Address 0x68
State computer;
PSRAM *ram;
#define BUZZER 33
#define BMP_ADDR_PIN 36

static double last = 0;//for better timing than "delay(100)"

void setup()
{
    // Setup BMP to use defualt address
    pinMode(BMP_ADDR_PIN, OUTPUT);
    digitalWrite(BMP_ADDR_PIN, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    ram = new PSRAM();

    computer.setBaro(&bmp);
    computer.setGPS(&gps);
    computer.setIMU(&bno);

    if (computer.init())
        recordLogData(INFO, "All Sensors Initialized");
    else
        recordLogData(ERROR, "Some Sensors Failed to Initialize. Disabling those sensors.");

    if (ram->init())
        recordLogData(INFO, "PSRAM Initialized");
    else
        recordLogData(ERROR, "PSRAM Failed to Initialize");

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    if (setupSDCard(computer.getcsvHeader()))
    {

        recordLogData(INFO, "SD Card Initialized");
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
    }
    else
    {
        recordLogData(ERROR, "SD Card Failed to Initialize");
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
    double time = millis();
    
    if(time - last < 100)
        return;

    last = time;
    computer.updateState();
    dataStageUpdate(computer.getStageNum());
    recordLogData(INFO, computer.getdataString(), TO_USB);
    recordFlightData(computer.getdataString());
}