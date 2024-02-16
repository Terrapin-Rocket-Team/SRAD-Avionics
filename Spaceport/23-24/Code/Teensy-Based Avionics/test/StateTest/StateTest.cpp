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

//column numbers, 0 indexed
int numColums = 17;
int AX = 3;//m/s/s
int AY = 4;
int AZ = 5;   // Z is up
int OX = 7;
int OY = 8;
int OZ = 9;
int OW = 6;
int BAlt = 10; // m
int BTemp = 12; // m
int BPres = 11; // m
int GX = 15;//lat/long
int GY = 14;
int GZ = 16;//Z is up

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
    recordData(computer.getdataString(), computer.stage);

    char* stateStr = computer.getStateString();
    Serial.println(stateStr);
    delete[] stateStr;
    stateStr = nullptr;

    delay(100);
    digitalWrite(BUZZER, LOW);
}

void ParseIncomingFakeSensorData(String line){
    if(line.length() == 0) return;
    String columns[numColums];
    int cursor = 0;
    for(int j = 0;j < line.length();j++){
        if(cursor == numColums) break;
        if(line[j] ==',')
        cursor++;
        else
        columns[cursor].append(line[j]);
    }
    baro.feedData((double)columns[BAlt].toFloat(),(double)columns[BTemp].toFloat(),(double)columns[BPres].toFloat());
    gps.feedData((double)columns[GX].toFloat(), (double)columns[GY].toFloat(), (double)columns[GZ].toFloat());
    fimu.feedData((double)columns[AX].toFloat(), (double)columns[AY].toFloat(), (double)columns[AZ].toFloat(), (double)columns[OX].toFloat(), (double)columns[OY].toFloat(), (double)columns[OZ].toFloat(), (double)columns[OW].toFloat());
}