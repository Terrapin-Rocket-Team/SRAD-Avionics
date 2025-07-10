#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "VoltageSensor.h"

using namespace mmfs;

MAX_M10S m;
DPS368 d;
BMI088Gyro g;
BMI088Accel a;
mmfs::LIS3MDL l;
VoltageSensor vsfc(A0, 330, 220, "Flight Computer Voltage");

Sensor *s[] = {&m, &d, &g, &a, &l, &vsfc};
AvionicsKF fk;
AvionicsState t(s, sizeof(s) / 4, &fk);

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

void FreeMem()
{
    void *heapTop = malloc(500);
    Serial.print((long)heapTop);
    Serial.print("\n");
    free(heapTop);
}

MMFSConfig c = MMFSConfig()
                   .withBBAsync(true, 50)
                   .withBBPin(LED_BUILTIN)
                   .withBBPin(32)
                   .withBuzzerPin(33)
                   .withUsingSensorBiasCorrection(true)
                   .withUpdateRate(10)
                   .withState(&t);
MMFSSystem sys(&c);

AviEventLister listener;

void setup()
{
    sys.init();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    getLogger().recordLogData(INFO_, "Initialization Complete");
}
void loop()
{
    sys.update();
}