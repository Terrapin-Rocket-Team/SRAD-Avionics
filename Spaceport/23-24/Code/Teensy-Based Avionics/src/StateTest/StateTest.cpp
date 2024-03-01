#include <Arduino.h>
#include "State.h"
#include "FakeBaro.h"
#include "FakeGPS.h"
#include "FakeIMU.h"
#include "TestUtils.h"
#include <RecordData.h>
#include <exception>
FakeBaro baro;
FakeGPS gps;
FakeIMU fimu;//"imu" is the namespace of the vector stuff :/
State computer(true);



int i = 0;



#define BUZZER 33

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    delay(2000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    computer.setBaro(&baro);
    computer.setGPS(&gps);
    computer.setIMU(&fimu);
    computer.init();

    setupPSRAM(computer.getcsvHeader());
    bool sdSuccess = setupSDCard(String(computer.getcsvHeader()));

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
    Serial.println("Initialized");
    //digitalWrite(LED_BUILTIN, HIGH); // keep light high indicating testing
    while (Serial.available() == 0)
    {
    }
    
}

void loop()
{
    if (i % 20 == 0 || i++ % 20 == 1)//infinitely beep buzzer to indicate testing state. Please do NOT launch rocket with test code on the MCU......
        digitalWrite(BUZZER, HIGH);  // buzzer is on for 200ms/2sec

    if(Serial.available() > 0){
        ParseIncomingFakeSensorData(Serial.readStringUntil('\n'),baro,gps,fimu);
    }

    computer.updateState();
    recordData(computer.getdataString(), computer.getStageNum());

    char* stateStr = computer.getStateString();
    Serial.print(FreeMem());
    Serial.println(stateStr);

    delay(50);
    digitalWrite(BUZZER, LOW);
}