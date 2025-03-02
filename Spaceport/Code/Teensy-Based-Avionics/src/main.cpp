#include <Arduino.h>
#include "BMI088.h"

#include "AvionicsState.h"
#include "Pi.h"
#include "AvionicsKF.h"
#include "RadioMessage.h"
#include "Si4463.h"
#include "AviEventListener.h"
#include <MMFS.h>

#define RPI_PWR 0
#define RPI_VIDEO 1

using namespace mmfs;

MAX_M10S gps;
mmfs::DPS310 baro1;
mmfs::BMI088andLIS3MDL bno;
Sensor *sensors[3] = {&gps, &bno, &baro1};
AvionicsKF kfilter;
AvionicsState computer(sensors, 3, &kfilter);

APRSConfig aprsConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint8_t encoding[] = {7, 4, 4};
APRSTelem aprs(aprsConfig);

Message msg;

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_500b,   // data rate
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

// Free memory debug function
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
// Free memory debug function

MMFSConfig config = MMFSConfig()
                        .withBBPin(LED_BUILTIN)
                        .withBBPin(32)
                        .withBuzzerPin(33)
                        .withState(&computer)
                        .withUsingSensorBiasCorrection(true)
                        .withUpdateRate(20)
                        .withBBAsync(true, 50);

MMFSSystem sys(&config);

AviEventLister listener;

void setup()
{
    // Serial1.begin(115200);
    sys.init();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)

    // if (radio.begin())
    // {
    //     bb.onoff(BUZZER, 1000);
    //     getLogger().recordLogData(ERROR_, "Radio initialized.");
    // }
    // else
    // {
    //     bb.onoff(BUZZER, 200, 3);
    //     getLogger().recordLogData(INFO_, "Radio failed to initialize.");
    // }

    getLogger().recordLogData(INFO_, "Initialization Complete");
}
double radio_last;
void loop()
{
    if (sys.update())
    {
        FreeMem();
    }
    // radio.update();

    double time = millis();
    if (time - radio_last < 1000)
        return;

    radio_last = time;
    msg.clear();

    /// printf("%f\n", baro1.getAGLAltFt());
    aprs.alt = baro1.getAGLAltFt();
    // printf("%f\n", gps.getHeading());
    aprs.hdg = gps.getHeading();
    // printf("%f\n", gps.getPos().x());
    aprs.lat = gps.getPos().x();
    // printf("%f\n", gps.getPos().y());
    aprs.lng = gps.getPos().y();
    // printf("%f\n", computer.getVelocity().z());
    aprs.spd = computer.getVelocity().z();
    // printf("%f\n", bno.getAngularVelocity().x());
    aprs.orient[0] = bno.getAngularVelocity().x();
    // printf("%f\n", bno.getAngularVelocity().y());
    aprs.orient[1] = bno.getAngularVelocity().y();
    // printf("%f\n", bno.getAngularVelocity().z());
    aprs.orient[2] = bno.getAngularVelocity().z();
    aprs.stateFlags.setEncoding(encoding, 3);

    uint8_t arr[] = {(uint8_t)(int)baro1.getTemp(), (uint8_t)computer.getStage(), (uint8_t)gps.getFixQual()};
    aprs.stateFlags.pack(arr);
    // aprs.stateFlags = (uint8_t) computer.getStage();
    msg.encode(&aprs);
    // radio.send(aprs);
    Serial.printf("%0.3f - Sent APRS Message; %f   |   %d\n", time / 1000.0, baro1.getAGLAltFt(), gps.getFixQual());
    bb.aonoff(BUZZER, 50);
    // Serial1.write(msg.buf, msg.size);
    // Serial1.write('\n');
}
