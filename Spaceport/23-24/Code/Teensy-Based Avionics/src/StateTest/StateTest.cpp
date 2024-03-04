#include <Arduino.h>
#include "State.h"
#include "FakeBaro.h"
#include "FakeGPS.h"
#include "FakeIMU.h"
#include "TestUtils.h"
#include <RecordData.h>
#include <exception>
#include "RFM69HCW.h"

FakeBaro baro;
FakeGPS gps;
FakeIMU fimu;//"imu" is the namespace of the vector stuff :/
State computer;
APRSConfig config = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
RadioSettings settings = {433.775, true, false, &hardware_spi, 10, 31, 32};
RFM69HCW radio = {settings, config};
PSRAM *ram;

int i = 0;



#define BUZZER 33

void setup()
{
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
    computer.setRadio(&radio);
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
static double radioTimer = 0;
bool more = false;
void loop()
{
    //if (i % 20 == 0 || i++ % 20 == 1)//infinitely beep buzzer to indicate testing state. Please do NOT launch rocket with test code on the MCU......
        //digitalWrite(BUZZER, HIGH);  // buzzer is on for 200ms/2sec

    if(Serial.available() > 0){
        ParseIncomingFakeSensorData(Serial.readStringUntil('\n'),baro,gps,fimu);
        Serial.clear();
    }
    if (millis() - radioTimer >= 1200)
    {
        more = computer.transmit();
        radioTimer = millis();
    }
    if (radio.mode() != RHGenericDriver::RHModeTx && more){
        more = !radio.sendBuffer();
    }
    computer.timeAbsolute = millis() / 1000.0;
    computer.updateState();

    char* stateStr = computer.getStateString();
    Serial.print("[][]");
    Serial.println(stateStr);
    delay(50);
    //digitalWrite(BUZZER, LOW);
}