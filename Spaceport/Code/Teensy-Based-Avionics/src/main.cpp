#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "Pi.h"
#include "Si4463.h"
#include "Radio/ESP32BluetoothRadio.h"
#include "VoltageSensor.h"

#include "422Mc80_4GFSK_009600H.h"

#define RPI_PWR 1
#define RPI_VIDEO 0

using namespace mmfs;

MAX_M10S m;
DPS368 d;
BMI088andLIS3MDL b;
VoltageSensor vsfc(A0, 330, 220, "Flight Computer Voltage");

Sensor *s[] = {&m, &d, &b, &vsfc};
AvionicsKF fk;
AvionicsState t(s, sizeof(s) / 4, &fk);

APRSConfig aprsConfigAvionics = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint8_t encoding[] = {7, 4, 4};
APRSConfig aprsConfigAirbrake = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
Message msgAvionics;
Message msgAirbrake;

ESP32BluetoothRadio btRad(Serial2, "AVIONICS", true);

Si4463HardwareConfig hwcfg = {
    MOD_4GFSK,        // modulation
    DR_4_8k,          // data rate
    (uint32_t)430e6,  // frequency (Hz)
    POWER_COTS_30dBm, // tx power (127 = ~20dBm)
    48,               // preamble length
    16,               // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    10,   // cs
    23,   // sdn
    20,   // irq
    21,   // gpio0
    22,   // gpio1
    36,   // gpio2
    37,   // gpio3
};

Si4463 radio(hwcfg, pincfg);
// uint32_t radioTimer = millis();
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
                      .withBuzzerPin(33)
                   .withUsingSensorBiasCorrection(true)
                   .withUpdateRate(10)
                   .withState(&t);
MMFSSystem sys(&a);

AviEventLister listener;

void setup()
{
    sys.init();
    Serial8.begin(115200);
    Serial2.begin(9600);
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)

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

    if (radio.begin(CONFIG_422Mc80_4GFSK_009600H, sizeof(CONFIG_422Mc80_4GFSK_009600H)))
    {
        bb.onoff(BUZZER, 1000);
        getLogger().recordLogData(ERROR_, "Radio initialized.");
    }
    else
    {
        bb.onoff(BUZZER, 200, 3);
        getLogger().recordLogData(INFO_, "Radio failed to initialize.");
    }

    getLogger().recordLogData(INFO_, "Initialization Complete");
}
uint32_t avionicsTimer = millis();
uint32_t airbrakeTimer = millis();
bool sendAirbrake = false;

void calcStuff();
Message mess;
APRSCmd cmd;

double motorTimer = 0;
int motorAngle = 0;
void loop()
{
    double timeeee = millis();
    if (millis() - motorTimer > 3000)
    {
        int mult = 1;
        if (motorAngle >= 360)
            mult = -1;
        else
            mult = 1;
        motorAngle += (mult * 90);
        Serial8.println(motorAngle);
    }

    if (btRad.isReady())
        Serial.print(btRad.isReady());
    // if (Serial2.available())
    //     Serial.write(Serial2.read());
    btRad.rx();
    if (millis() > 44 * 1000 * 60 && !pi.isOn())
    {
        pi.setOn(true);
    }
    // if (millis() > 1 * 1000 * 60 && !pi.isRecording())
    // {
    //     pi.setRecording(true);
    // }
    // if (radio.avail())
    // {
    //     radio.readRXBuf(mess.buf, mess.maxSize);
    //     if (!strcmp((char *)mess.buf, "KD3BBD"))
    //     {
    //         mess.decode(&cmd);
    //         if (cmd.cmd == 1)
    //         {
    //             pi.setOn(cmd.args.get());
    //         }
    //         else if (cmd.cmd == 2)
    //         {
    //             pi.setRecording(cmd.args.get());
    //         }
    //         else if (cmd.cmd == 8)
    //         {
    //             if (!(t.getStage() == 1 || t.getStage() == 2))
    //                 Serial2.printf("%d\n", cmd.args.get());
    //         }
    //     }
    // }

    if (sys.update())
    {
        calcStuff();
        if(!pi.isRecording() && t.getStage() > 0)
            pi.setRecording(true);
        if (btRad.getReceiveSize() > 0)
        {
            APRSTelem ab(aprsConfigAirbrake);
            char asdf[100];
            int i = btRad.readBuffer(asdf, 100);
            ab.decode((uint8_t *)asdf, i);
            // Serial.println("Decode");
            if (!ab.err)
            {
                msgAirbrake.size = i;
                memcpy(msgAirbrake.buf, asdf, i);
                // msgAirbrake.decode(&ab);
                // Serial.write(msgAirbrake.buf, msgAirbrake.size);
                // Serial.println();
            }
            else
            {
                Serial.println("error");
            }
            // Serial.write(asdf, i);
            // Serial.println();
            // for (int i = 0; i < msgAirbrake.size; i++)
            // {
            //     Serial.write((char *)msgAirbrake.buf, msgAirbrake.size);
            // }
            // Serial.println();
        }
    }

    if (millis() - avionicsTimer > 1000)
    {
        avionicsTimer = millis();
        sendAirbrake = true;
        // msg.clear();

        double orient[3] = {b.getAngularVelocity().x(), b.getAngularVelocity().y(), b.getAngularVelocity().z()};
        APRSTelem aprs = APRSTelem(aprsConfigAvionics, m.getPos().x(), m.getPos().y(), d.getAGLAltFt(), t.getVelocity().z() * 3.28, m.getHeading(), orient, 0);

        aprs.stateFlags.setEncoding(encoding, 3);
        uint8_t arr[] = {(uint8_t)(int)d.getTemp(), (uint8_t)t.getStage(), (uint8_t)m.getFixQual()};
        aprs.stateFlags.pack(arr);
        msgAvionics.encode(&aprs);
        // Serial.print("sending");
        radio.send(aprs);
        // Serial.println("sending");

        // Serial.printf("%d %ld\n", d.getTemp(), aprs.stateFlags.get());

        // Serial.printf("%0.3f - Sent APRS Message; %f   |   %d\n", time / 1000.0, d.getAGLAltFt(), m.getFixQual());
        // bb.aonoff(BUZZER, 50);
        // Serial.write(msgAvionics.buf, msgAvionics.size);
        // Serial.write('\n');
    }
    if (sendAirbrake && millis() - avionicsTimer > 300 && millis() - avionicsTimer < 400 && millis())
    {
        sendAirbrake = false;
        // double orient[3] = {b.getAngularVelocity().x(), b.getAngularVelocity().y(), b.getAngularVelocity().z()};
        // APRSTelem aprs = APRSTelem(aprsConfigAirbrake, m.getPos().x(), m.getPos().y(), d.getAGLAltFt(), t.getVelocity().z(), m.getHeading(), orient, 0);

        // aprs.stateFlags.setEncoding(encoding, 3);
        // uint8_t arr[] = {(uint8_t)(int)d.getTemp(), (uint8_t)t.getStage(), (uint8_t)m.getFixQual()};
        // aprs.stateFlags.pack(arr);
        // msgAvionics.encode(&aprs);
        // radio.send(aprs);
        radio.tx(msgAirbrake.buf, msgAirbrake.size);
        // Serial.println("sending");
        // Serial.println(msgAirbrake.size);
        // Serial.write(msgAirbrake.buf, msgAirbrake.size);
        // Serial.println();
        // msgAirbrake.size = 0;
    }

    // char str[512];
    // int i = snprintf(str, 512, "La %.7f Lo %.7f Al %.2f Hd %.2f Ql %d", 1.0, 1.0, 2.0, 360.0, 5);
    // snprintf(str, 512, "1234567890123456789");
    // btRad.tx("Hello", 5);
    // btRad.tx((uint8_t *)str, strlen(str));
    // Serial.printf("sent %d\n", strlen(str));

    // /// printf("%f\n", baro1.getAGLAltFt());

    radio.update();
    if (millis() - timeeee > 30)
        Serial.println(millis() - timeeee);
}
int counter = 0;
void calcStuff()
{
    if (t.getStage() == 3 || t.getStage() == 4)
    {
        if (t.getTimeSinceLastStage() > 3 && counter == 0)
        {
            Serial.println("first");
            Serial8.println("90");
            counter++;
        }
        else if (t.getTimeSinceLastStage() > 5 && counter <= 1)
        {
            counter++;
            Serial8.println("180");
        }
        else if (t.getTimeSinceLastStage() > 20 && counter <= 2)
        {
            Serial8.println("360");
            counter++;
        }
        else if (t.getTimeSinceLastStage() > 30 && counter <= 3)
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
    else if (t.getStage() == 4 && t.getTimeSinceLastStage() > 10 && counter <= 5)
    {
        Serial8.println("0");
        counter++;
    }
}