#include <Arduino.h>
#include "AvionicsState.cpp"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "Storage/recordData.h"

BNO055 bno(13, 12);   //I2C Address 0x29
BMP390 bmp(13, 12);   //I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); //I2C Address 0x42  
DS3231 rtc();   //I2C Address 0x68
AvionicsState computer;

#define BUZZER 33

void setup() {
    Serial.begin(9600);
    while (!Serial);

    pinMode(BUZZER, OUTPUT);
    
    computer.addBarometer(&bmp);
    computer.addGPS(&gps);
    // computer.addRTC(&rtc);
    computer.addIMU(&bno);

    computer.stateBarometer->initialize();
    computer.stateGPS->initialize();
    computer.stateIMU->initialize();
    // computer.stateRTC->initialize();

    computer.setcsvHeader();
    setupPSRAM(computer.csvHeader);
    bool sdSuccess = setupSDCard(computer.csvHeader);

    if (sdSuccess) {
        Serial.println("SD Card initialized");
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
    } else {
        Serial.println("SD Card failed to initialize");
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(200);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);

    }

}

void loop() {
    
    computer.updateSensors();
    computer.updateState();

    computer.setdataString();
    recordData(computer.getdataString(), computer.getrecordDataState());

}