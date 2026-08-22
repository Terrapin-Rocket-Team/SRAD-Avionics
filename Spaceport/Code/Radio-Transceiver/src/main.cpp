#include <Arduino.h>
#include "Si4464.h"
#include "List.h"
#include "RadioMessage.h"

#define IS_ACTIVE_NODE true
#define RX_TIMEOUT 100     // ms
#define CHANNEL 40         // Freq = 220 + (CHANNEL * 0.1) MHz
#define SERIAL_BAUD 115200 // bits/s
#define END_CHAR '\n'

const uint8_t STAT_PIN = (uint8_t)pinNametoDigitalPin(PA_11);

// Base Freq: 220MHz
// Channel Step: 100kHz
// Channel Number: 10
// Carrier Freq: 221MHz

Si4464HardwareConfig hwcfg = {
    MOD_4GFSK,       // modulation
    DR_100k,         // data rate
    CHANNEL,         // channel
    POWER_MPM_30dBm, // tx power
    192,             // preamble length
    32               // required received valid preamble
};

Si4464PinConfig pincfg = {
    &SPI,                               // spi bus to use
    (uint8_t)pinNametoDigitalPin(PB_0), // cs
    (uint8_t)pinNametoDigitalPin(PA_3), // sdn
    (uint8_t)pinNametoDigitalPin(PA_4), // irq
    (uint8_t)pinNametoDigitalPin(PB_1), // gpio0
    (uint8_t)pinNametoDigitalPin(PA_0), // gpio1
    (uint8_t)pinNametoDigitalPin(PA_1), // gpio2
    (uint8_t)pinNametoDigitalPin(PA_2), // gpio3
};

Si4464 radHW(hwcfg, pincfg);

Radio radio(&radHW, IS_ACTIVE_NODE, RX_TIMEOUT);

HardwareSerial Serial1(PA_9, PA_10);

Message m;
bool serialReadLock = false;

uint32_t ledTimer = millis();
uint8_t g_times = 0;
uint32_t g_interval = 0;
bool ledOn = false;
void blink(uint8_t times, uint32_t interval);
void updateBlink();

uint8_t testBuf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

void setup()
{
    // set correct SPI pins
    SPI.setMISO(PA_6);
    SPI.setMOSI(PA_7);
    SPI.setSCLK(PA_5);

    Serial1.begin(SERIAL_BAUD);
    pinMode(STAT_PIN, OUTPUT);

    Serial1.println("Starting radio");

    if (!radHW.begin())
    {
        Serial1.println("ERROR: radio failed to begin");
        while (true)
        {
            digitalWrite(STAT_PIN, HIGH);
            delay(1000);
            digitalWrite(STAT_PIN, LOW);
            delay(1000);
        }
    }

    Serial1.println("Radio Started");

    delay(1000);

    digitalWrite(STAT_PIN, HIGH);
    delay(500);
    digitalWrite(STAT_PIN, LOW);
    delay(500);

    // flush serial
    while (Serial1.available())
        Serial1.read();

    // pinMode((uint8_t)pinNametoDigitalPin(PB_1), OUTPUT);
    // digitalWrite((uint8_t)pinNametoDigitalPin(PB_1), HIGH);

    // TODO: setup debug serial
}

uint32_t timer = millis();

void loop()
{
    if (millis() - timer > 1000)
    {
        timer = millis();
        radHW.tx(testBuf, sizeof(testBuf));
        Serial1.println("TX");
    }

    // // check for input to be transmitted
    // while (Serial1.available() && m.size < Message::maxSize)
    // {
    //     // will only occur when first getting serial data
    //     if (!serialReadLock)
    //         serialReadLock = true;
    //     // read in character
    //     char c = Serial1.read();

    //     // denote complete messages by \n for now (text only)
    //     if (c == END_CHAR)
    //     {
    //         // send complete message
    //         radio.send(&m);
    //         m.clear();
    //         serialReadLock = false;
    //         blink(1, 100);
    //         break;
    //     }
    //     else
    //         m.append(c);
    // }

    // // check for received messages
    // if (!serialReadLock && radio.receive(&m))
    // {
    //     m.print(Serial1);
    //     m.clear();
    //     blink(2, 100);
    // }

    // // if somehow we reach the max message size, dump the data to prevent lockup
    // if (m.size == Message::maxSize)
    //     m.clear();

    // radio update
    radio.update();
    // updateBlink();
}

void blink(uint8_t times, uint32_t interval)
{
    g_times = times;
    g_interval = interval;
    ledTimer = millis();
}

// this will not work particularly well without queueing
// but it makes light go flashy if data sent so it work
void updateBlink()
{
    if ((millis() - ledTimer) % g_interval < 5 && g_times >= 0)
    {
        ledOn != ledOn;
        digitalWrite(STAT_PIN, ledOn ? HIGH : LOW);
        // only decrement times after 1 cycle
        if (ledOn)
            g_times--;
    }
}