#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "Pi.h"
#include "Si4463.h"
#include "Radio/ESP32BluetoothRadio.h"

#define RPI_PWR 24
#define RPI_VIDEO 25

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

ESP32BluetoothRadio btRad(Serial1, "AVIONICS", true);

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_100k,   // data rate
    433e6,     // frequency (Hz)
    5,         // tx power (127 = ~20dBm)
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
Pi pi(RPI_PWR, RPI_VIDEO);

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
                   .withUpdateRate(5)
                   .withState(&t);
MMFSSystem sys(&a);

AviEventLister listener;

void setup()
{
    sys.init();
    // Serial1.begin(9600);
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    // if (radio.begin())
    // {
    //     bb.onoff(BUZZER, 1000); // 1 x 1 sec beep for sucessful initialization
    //     getLogger().recordLogData(INFO_, "Initialized Radio");
    // }
    // else
    // {
    //     bb.onoff(BUZZER, 2000, 3); // 3 x 2 sec beep for uncessful initialization
    //     getLogger().recordLogData(ERROR_, "Initialized Radio Failed");
    // }

    if (btRad.begin())
    {
        bb.onoff(BUZZER, 500); // 1 x 0.5 sec beep for sucessful initialization
        getLogger().recordLogData(INFO_, "Initialized Bluetooth");
    }
    else
    {
        bb.onoff(BUZZER, 1000, 3); // 3 x 2 sec beep for uncessful initialization
        getLogger().recordLogData(ERROR_, "Initialized Bluetooth Failed");
    }
    getLogger().recordLogData(INFO_, "Initialization Complete");
}
double radio_last;
void calcStuff();
void loop()
{
    // btRad.rx();
    if (btRad.isReady())
        Serial.print(btRad.isReady());
    if (Serial1.available())
        Serial.write(Serial1.read());
    if (t.getStage() > 0)
        pi.setRecording(true);
    // if (millis() > 15 * 1000)
    //     pi.setRecording(false);

    // radio.update();
    if (sys.update())
    {
    }

    double time = millis();
    if (time - radio_last < 2000)
        return;

    char str[512];
    // int i = snprintf(str, 512, "La %.7f Lo %.7f Al %.2f Hd %.2f Ql %d", 1.0, 1.0, 2.0, 360.0, 5);
    snprintf(str, 512, "1234567890123456789");
    // btRad.tx("Hello", 5);
    // btRad.tx((uint8_t *)str, strlen(str));
    // Serial.printf("sent %d", strlen(str));
    // calcStuff();
    radio_last = time;
    msg.clear();
    // radio.update();

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
    // Serial.printf("%d %ld\n", d.getTemp(), aprs.stateFlags.get());
    msg.encode(&aprs);
    // radio.send(aprs);
    // Serial.printf("%0.3f - Sent APRS Message; %f   |   %d\n", time / 1000.0, d.getAGLAltFt(), m.getFixQual());
    // bb.aonoff(BUZZER, 50);
    // Serial1.write(msg.buf, msg.size);
    // Serial1.write('\n');
}
int counter = 0;
void calcStuff()
{
    if (t.getStage() == 3)
    {
        if (t.getTimeSinceLastStage() > 3 && counter == 0)
        {
            Serial.println("first");
            Serial8.println("90");
            counter++;
        }
        else if (t.getTimeSinceLastStage() > 6 && counter <= 1)
        {
            counter++;
            Serial8.println("180");
        }
        else if (t.getTimeSinceLastStage() > 30 && counter <= 2)
        {
            Serial8.println("360");
            counter++;
        }
        else if (t.getTimeSinceLastStage() > 300 && counter <= 3)
        {
            Serial8.println("180");
            counter++;
        }
    }
    else if (t.getStage() == 4 && counter <= 4)
    {
        Serial8.println("180");
        counter++;
    }
    else if (t.getStage() == 4 && t.getTimeSinceLastStage() > 30 && counter <= 5)
    {
        Serial.println("0");
        counter++;
    }
}