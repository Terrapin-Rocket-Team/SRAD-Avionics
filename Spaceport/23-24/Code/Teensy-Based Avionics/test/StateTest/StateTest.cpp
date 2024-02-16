#include <Arduino.h>
#include "State.h"
#include "FakeBaro.h"
#include "FakeGPS.h"
#include "FakeIMU.h"
#include <RecordData.h>

FakeBaro baro;
FakeGPS gps;
FakeIMU fimu;//"imu" is the namespace of the vector stuff :/
State computer;


int i = 0;

//column numbers
int AX = 0;//m/s/s
int AY = 0;
int AZ = 0;//Z is up
int BAlt = 0;//m
int GX = 0;//displacement from start in m
int GY = 0;
int GZ = 0;//Z is up

#define BUZZER 33

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // keep light high indicating testing

    Serial.begin(9600);
    while (!Serial)
        ;

    computer.setBaro(&baro);
    computer.setGPS(&gps);
    // computer.setRTC(&rtc);
    computer.setIMU(&fimu);

    computer.init();
    setupPSRAM(computer.csvHeader);
    bool sdSuccess = setupSDCard(computer.csvHeader);

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
    if (i % 20 == 0 || i++ % 20 == 1)//infinitely beep buzzer to indicate testing state. Please do NOT launch rocket with test code on the MCU......
        digitalWrite(BUZZER, HIGH);  // buzzer is on for 200ms/2sec

    if(Serial.available() > 0)
        ParseIncomingFakeSensorData(Serial.readStringUntil('\n'));

    computer.updateState();

    // Serial.println(computer.getdataString());
    recordData(computer.getdataString(), computer.stage);
    delay(100);
    digitalWrite(BUZZER, LOW);
}

void ParseIncomingFakeSensorData(String line){
    if(line.length() > 0);
}