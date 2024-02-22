#include <Arduino.h>
#include "State.h"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include <RecordData.h>

BNO055 bno(13, 12);   //I2C Address 0x29
BMP390 bmp(13, 12);   //I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); //I2C Address 0x42  
DS3231 rtc();   //I2C Address 0x68
State computer;

#define BUZZER 33

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);


    // Serial.begin(9600);
    // while (!Serial);

    
    computer.setBaro(&bmp);
    // computer.addGPS(&gps);
    // computer.addRTC(&rtc);
    computer.setIMU(&bno);

    computer.init();
    setupPSRAM(computer.getcsvHeader());
    bool sdSuccess = setupSDCard(computer.getcsvHeader());

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    if (sdSuccess) {
        // Serial.println("SD Card initialized");
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
    } else {
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

void loop() {
    
    computer.updateState();

    // Serial.println(computer.getdataString());
    recordData(computer.getdataString(), computer.getStageNum());
    delay(50);
}