#include <Arduino.h>
#include "Si4464.h"
#include "List.h"

#define MSG_SIZE_OVRD 200
#include "RadioMessage.h"

#define IS_ACTIVE_NODE true
#define RX_TIMEOUT 100     // ms
#define CHANNEL 40         // Freq = 220 + (CHANNEL * 0.1) MHz
#define SERIAL_BAUD 115200 // bits/s
#define END_CHAR '0'

const uint8_t STAT_PIN = (uint8_t)pinNametoDigitalPin(PA_11);

// Base Freq: 220MHz
// Channel Step: 100kHz
// Channel Number: 10
// Carrier Freq: 221MHz

Si4464HardwareConfig hwcfg = {
    MOD_4GFSK, // modulation
    DR_100k,   // data rate
    CHANNEL,   // channel
    1,         // tx power
    192,       // preamble length
    32         // required received valid preamble
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

HardwareSerial Serial1(PA_10, PA_9);

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

    delay(1000);
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

    digitalWrite(STAT_PIN, HIGH);
    delay(500);
    digitalWrite(STAT_PIN, LOW);
    delay(500);
    Serial1.println("Radio Started");

    // flush serial
    while (Serial1.available())
        Serial1.read();

    // TODO: setup debug serial
}

uint32_t timer = millis();

void loop()
{
    // if (millis() - timer > 1000)
    // {
    //     timer = millis();
    //     radHW.tx(testBuf, sizeof(testBuf));
    //     // Serial.println("TX");
    // }

    // Serial.println("main.cpp before tx");
    // Serial.flush();

    // check for input to be transmitted
    while (Serial1.available() && m.size < Message::maxSize)
    {
        // will only occur when first getting serial data
        if (!serialReadLock)
            serialReadLock = true;
        // read in character
        char c = Serial1.read();

        // denote complete messages by \n for now (text only)
        if (c == END_CHAR)
        {
            Serial1.println("main.cpp Sending msg ");
            Serial1.write(m.buf, m.size);
            Serial1.flush();
            delay(1000);
            // send complete message
            radio.send(&m);
            Serial1.println("\nafter send");
            Serial1.flush();
            delay(1000);

            m.clear();
            serialReadLock = false;
            // blink(1, 100);
            break;
        }
        else
            m.append(c);
    }

    // Serial.println("main.cpp before rx");
    // Serial.flush();

    // check for received messages
    // if (!serialReadLock && radio.receive(&m))
    // {
    //     // Serial.println("main.cpp");
    //     // Serial.flush();
    //     // m.write(Serial1);
    //     for (int i = 0; i < m.size; i++)
    //     {
    //         Serial1.print(m.buf[i]);
    //         Serial1.print(" ");
    //     }
    //     Serial1.println();
    //     m.clear();
    //     Serial1.print("RSSI: ");
    //     Serial1.println(radHW.RSSI());
    //     // m.fill((uint8_t *)"ACK ", sizeof("ACK "));
    //     // radio.send(&m);
    //     // m.clear();
    //     // blink(2, 100);
    // }
    // Serial.println("main.cpp after rx");
    // Serial.flush();

    // if somehow we reach the max message size, dump the data to prevent lockup
    // if (m.size == Message::maxSize)
    //     m.clear();

    // Serial.println("main.cpp before update");
    // Serial.flush();

    // radio update
    radio.update();
    // updateBlink();

    // Serial.println("main.cpp after update");
    // Serial.flush();
}

void blink(uint8_t times, uint32_t interval)
{
    g_times = times;
    g_interval = interval;
    ledTimer = millis();
    ledOn = true;
    digitalWrite(STAT_PIN, HIGH);
}

// this will not work particularly well without queueing
// but it makes light go flashy if data sent so it work
void updateBlink()
{
    if (millis() - ledTimer > g_interval && g_times > 0)
    {
        ledTimer = millis();
        ledOn = !ledOn;
        digitalWrite(STAT_PIN, ledOn ? HIGH : LOW);
        // only decrement times after 1 cycle
        if (ledOn)
            g_times--;
    }
    if (g_times == 0)
    {
        digitalWrite(STAT_PIN, LOW);
    }
}