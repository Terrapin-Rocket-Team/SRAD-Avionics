#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "Pi.h"
#include "Si4463.h"

#define RPI_PWR 0
#define RPI_VIDEO 1

using namespace mmfs;


MAX_M10S m;
DPS310 d;
BMI088andLIS3MDL b;

Sensor *s[] = {&m, &d, &b};
AvionicsKF fk;
AvionicsState t(s, sizeof(s) / 4, &fk);



APRSConfig aprsConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint8_t encoding[] = {7, 4, 4};
APRSTelem aprs(aprsConfig);
Message msg;

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_100k,   // data rate
    433e6,     // frequency (Hz)
    5,       // tx power (127 = ~20dBm)
    48,        // preamble length
    16,        // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    10,   // cs
    20,   // sdn
    23,   // irq
    22,   // gpio0
    21,   // gpio1
    36,   // random pin - gpio2 is not connected
    37,   // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
uint32_t radioTimer = millis();
Pi rpi(RPI_PWR, RPI_VIDEO);

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

MMFSConfig a = MMFSConfig()
                   .withBBAsync(true, 50)
                   .withBBPin(LED_BUILTIN)
                   .withBBPin(32)
                //    .withBuzzerPin(33)
                   .withUsingSensorBiasCorrection(true)
                   .withUpdateRate(10)
                   .withState(&t);
MMFSSystem sys(&a);

AviEventLister listener;

void setup()
{
    sys.init();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    if (radio.begin()) { 
        bb.onoff(BUZZER, 1000); // 1 x 1 sec beep for sucessful initialization
        getLogger().recordLogData(INFO_, "Initialized Radio");
        
    } else {
        bb.onoff(BUZZER, 2000, 3); // 3 x 2 sec beep for uncessful initialization
        getLogger().recordLogData(ERROR_, "Initialized Radio Failed");
    }
    getLogger().recordLogData(INFO_, "Initialization Complete");
}  
double radio_last;

void loop()
{
    if (sys.update())
    {
    }
    double time = millis();
    if (time - radio_last < 1000)
        return;

    radio_last = time;
    msg.clear();
    radio.update();

    /// printf("%f\n", baro1.getAGLAltFt());
    aprs.alt = d.getAGLAltFt();
    // printf("%f\n", gps.getHeading());
    aprs.hdg = m.getHeading();
    // printf("%f\n", gps.getPos().x());
    aprs.lat = m.getPos().x();
    // printf("%f\n", gps.getPos().y());
    aprs.lng = m.getPos().y();
    // printf("%f\n", computer.getVelocity().z());
    aprs.spd = t.getVelocity().z();
    // printf("%f\n", bno.getAngularVelocity().x());
    aprs.orient[0] = b.getAngularVelocity().x();
    // printf("%f\n", bno.getAngularVelocity().y());
    aprs.orient[1] = b.getAngularVelocity().y();
    // printf("%f\n", bno.getAngularVelocity().z());
    aprs.orient[2] = b.getAngularVelocity().z();
    aprs.stateFlags.setEncoding(encoding, 3);

    uint8_t arr[] = {(uint8_t)(int)d.getTemp(), (uint8_t)t.getStage(), (uint8_t)m.getFixQual()};
    aprs.stateFlags.pack(arr);
    Serial.printf("%d %ld\n", d.getTemp(), aprs.stateFlags.get());
    // aprs.stateFlags = (uint8_t) computer.getStage();
    msg.encode(&aprs);
    radio.send(aprs);
    Serial.printf("%0.3f - Sent APRS Message; %f   |   %d\n", time / 1000.0, d.getAGLAltFt(), m.getFixQual());
    bb.aonoff(BUZZER, 50);
    // Serial1.write(msg.buf, msg.size);
    // Serial1.write('\n');

}