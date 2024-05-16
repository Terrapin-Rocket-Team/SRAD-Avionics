#include <Arduino.h>
#include "State.h"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "RFM69HCW.h"
#include "RecordData.h"
#include "BlinkBuzz.h"

BNO055 bno(13, 12);         // I2C Address 0x29
BMP390 bmp(13, 12);         // I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); // I2C Address 0x42
RadioSettings settings = {915.0, true, false, &hardware_spi, 10, 31, 32};
RFM69HCW radio(&settings);
State computer; // = useKalmanFilter = true, stateRecordsOwnData = true
uint32_t radioTimer = millis();

PSRAM *ram;

#define BUZZER 33
#define BMP_ADDR_PIN 36
#define RPI_PWR 0
#define RPI_VIDEO 1

static double last = 0; // for better timing than "delay(100)"

// BlinkBuzz setup
int allowedPins[] = {LED_BUILTIN};
BlinkBuzz bb(allowedPins, 1, true);
extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

void FreeMem()
{
    void *heapTop = malloc(50);
    Serial.print((long)heapTop);
    Serial.print(" ");
    free(heapTop);
}

void setup()
{

    pinMode(LED_BUILTIN, OUTPUT);
    bb.onoff(BUZZER, 100, 4, 100);
    recordLogData(INFO, "Initializing Avionics System. 5 second delay to prevent unnecessary file generation.", TO_USB);
    // delay(5000);

    pinMode(BMP_ADDR_PIN, OUTPUT);
    digitalWrite(BMP_ADDR_PIN, HIGH);

    pinMode(RPI_PWR, OUTPUT);   // RASPBERRY PI TURN ON
    pinMode(RPI_VIDEO, OUTPUT); // RASPBERRY PI TURN ON

    digitalWrite(RPI_PWR, LOW);
    digitalWrite(RPI_VIDEO, HIGH);

    bb.onoff(LED_BUILTIN, 100);
    ram = new PSRAM(); // init after the SD card for better data logging.

    // The SD card MUST be initialized first to allow proper data logging.
    if (setupSDCard())
    {

        recordLogData(INFO, "SD Card Initialized");
        bb.onoff(BUZZER, 1000);
    }
    else
    {
        recordLogData(ERROR, "SD Card Failed to Initialize");

        bb.onoff(BUZZER, 200, 3);
    }

    // The PSRAM must be initialized before the sensors to allow for proper data logging.

    if (ram->init())
        recordLogData(INFO, "PSRAM Initialized");
    else
        recordLogData(ERROR, "PSRAM Failed to Initialize");

    if (!computer.addSensor(&bmp))
        recordLogData(INFO, "Failed to add BMP390 Sensor");
    if (!computer.addSensor(&gps))
        recordLogData(INFO, "Failed to add MAX_M10S Sensor");
    if (!computer.addSensor(&bno))
        recordLogData(INFO, "Failed to add BNO055 Sensor");
    computer.setRadio(&radio);
    if (computer.init())
    {
        recordLogData(INFO, "All Sensors Initialized");
        bb.onoff(BUZZER, 1000);
    }
    else
    {
        recordLogData(ERROR, "Some Sensors Failed to Initialize. Disabling those sensors.");
        bb.onoff(BUZZER, 200, 3);
    }
    sendSDCardHeader(computer.getCsvHeader());
}
static bool first = false;
static bool second = false;
void loop()
{
    bb.update();
    radio.update();
    double time = millis();

    if (time - radioTimer >= 1000)
    {
        computer.transmit();
        radioTimer = time;
    }
    if (time - last < 100)
        return;

    last = time;
    computer.updateState();
    //recordLogData(INFO, computer.getStateString(), TO_USB);
    // RASPBERRY PI TURN ON
    if (time / 1000.0 > 810)
    {
        digitalWrite(RPI_PWR, HIGH);
        if (!first)
        {
            bb.aonoff(BUZZER, 100, 2);
            first = true;
        }
    }
    if (computer.getStageNum() >= 1)
    {
        digitalWrite(RPI_VIDEO, LOW);
        if (!second)
        {
            bb.aonoff(BUZZER, 100, 3);
            second = true;
        }
    }
    // if (time / 1000.0 > 180)
    // {
    //     digitalWrite(RPI_VIDEO, LOW);
    //     if (!second)
    //     {
    //         bb.aonoff(BUZZER, 100, 3);
    //         second = true;
    //     }
    // }
}
