#include <Arduino.h>
#include "State.h"
#include "FakeBaro.h"
#include "FakeGPS.h"
#include "FakeIMU.h"
#include "TestUtils.h"
#include "RecordData.h"
#include <exception>
#include "RFM69HCW.h"

FakeBaro baro;
FakeGPS gps;
FakeIMU fimu;//"imu" is the namespace of the vector stuff :/
State computer;
PSRAM *ram;

int i = 0;



#define BUZZER 33

int allowedPins[] = {LED_BUILTIN, BUZZER};
BlinkBuzz bb(allowedPins, 2, true);

void setup()
{
    FreeMem();
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER, OUTPUT); //its very loud during testing

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    ram = new PSRAM(); // init after the SD card for better data logging.
    // The SD card MUST be initialized first to allow proper data logging.
    if (setupSDCard())
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

    // The PSRAM must be initialized before the sensors to allow for proper data logging.

    if (ram->init())
        recordLogData(INFO, "PSRAM Initialized");
    else
        recordLogData(ERROR, "PSRAM Failed to Initialize");

    computer.addSensor(&baro);
    computer.addSensor(&gps);
    computer.addSensor(&fimu);
    //computer.setRadio(&radio);
    if (computer.init())
        recordLogData(INFO, "All Sensors Initialized");
    else
        recordLogData(ERROR, "Some Sensors Failed to Initialize. Disabling those sensors.");

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    sendSDCardHeader(computer.getCsvHeader());
    while(Serial.available() == 0);
}
void loop()
{
    double t = 1838.75;
    if(Serial.available() > 0){
        t = ParseIncomingFakeSensorData(Serial.readStringUntil('\n'), baro, gps, fimu);
        computer.updateState(t - 1838.76); // starting time of fake data
        char *stateStr = computer.getStateString();
        Serial.print("[][]");
        Serial.println(stateStr);
        delay(40);
    }
}