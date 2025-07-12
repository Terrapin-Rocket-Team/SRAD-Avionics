#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "VoltageSensor.h"
#include "ADXL375.h"

void FreeMem();

using namespace mmfs;

MAX_M10S m;

DPS368 d;
BMI088Gyro g;
BMI088Accel a;
mmfs::LIS3MDL l;
VoltageSensor vsfc(A0, 47000, 4700, "Flight Computer Voltage");
ADXL375 accel;

Sensor *s[] = {&m, &d, &g, &a, &l, &vsfc, &accel};

AvionicsKF fk;
AvionicsState t(s, sizeof(s) / 4, &fk);

MMFSConfig c = MMFSConfig()
                   .withBBAsync(true, 50)
                   .withBBPin(LED_BUILTIN)
                   .withBBPin(32)
                   .withBuzzerPin(33)
                   .withUsingSensorBiasCorrection(true)
                   .withUpdateRate(50)
                   .withLoggingRate(10)
                   .withState(&t);
MMFSSystem sys(&c);

AviEventLister listener;

void setup()
{
    Serial.begin(115200);
    sys.init();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    getLogger().recordLogData(INFO_, "Initialization Complete");
}
void loop()
{

    if (sys.update())
    {
    }
}

void FreeMem()
{
    void *heapTop = malloc(500);
    Serial.print((long)heapTop);
    Serial.print("\n");
    free(heapTop);
}