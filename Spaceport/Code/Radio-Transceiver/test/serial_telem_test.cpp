#include <Arduino.h>
#include "MockRadio.h"
#include "List.h"
#include "RadioMessage.h"

// #define IS_ACTIVE_NODE true
#define IS_ACTIVE_NODE false
#define RX_TIMEOUT 100     // ms
#define CHANNEL 40         // Freq = 220 + (CHANNEL * 0.1) MHz
#define SERIAL_BAUD 115200 // bits/s
#define END_CHAR '0'

// const uint8_t STAT_PIN = (uint8_t)pinNametoDigitalPin(PA_11);

// Base Freq: 220MHz
// Channel Step: 100kHz
// Channel Number: 10
// Carrier Freq: 221MHz

MockHardwareConfig hwcfg = {
    115200,
};

MockPinConfig pincfg = {
    &Serial1,
};

MockRadio radHW(hwcfg, pincfg);

Radio radio(&radHW, IS_ACTIVE_NODE, RX_TIMEOUT);

// HardwareSerial Serial1(PA_9, PA_10);

Message m;
bool serialReadLock = false;

uint32_t ledTimer = millis();
uint8_t g_times = 0;
uint32_t g_interval = 0;
bool ledOn = false;
void blink(uint8_t times, uint32_t interval);
void updateBlink();

void setup()
{
    // set correct SPI pins
    // SPI.setMISO(PA_6);
    // SPI.setMOSI(PA_7);
    // SPI.setSCLK(PA_5);

    // Serial1.begin(SERIAL_BAUD);
    // pinMode(STAT_PIN, OUTPUT);
    Serial.begin(115200);

    if (CrashReport)
        Serial.println(CrashReport);
    Serial.println("Starting radio");

    if (!radHW.begin())
    {
        Serial.println("ERROR: radio failed to begin");
        while (true)
        {
            // digitalWrite(STAT_PIN, HIGH);
            // delay(1000);
            // digitalWrite(STAT_PIN, LOW);
            delay(1000);
        }
    }

    Serial.println("Radio Started");

    delay(1000);

    // digitalWrite(STAT_PIN, HIGH);
    // delay(500);
    // digitalWrite(STAT_PIN, LOW);
    // delay(500);

    // flush serial
    while (Serial.available())
        Serial.read();

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
    while (Serial.available() && m.size < Message::maxSize)
    {
        // will only occur when first getting serial data
        if (!serialReadLock)
            serialReadLock = true;
        // read in character
        char c = Serial.read();

        // denote complete messages by \n for now (text only)
        if (c == END_CHAR)
        {
            Serial.println("main.cpp Sending msg ");
            Serial.write(m.buf, m.size);
            Serial.flush();
            // send complete message
            radio.send(&m);
            Serial.println("\n after send");
            Serial.flush();
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
    if (!serialReadLock && radio.receive(&m))
    {
        // Serial.println("main.cpp");
        // Serial.flush();
        m.print(Serial);
        m.clear();
        // m.fill((uint8_t *)"ACK ", sizeof("ACK "));
        // radio.send(&m);
        // m.clear();
        // blink(2, 100);
    }
    // Serial.println("main.cpp after rx");
    // Serial.flush();

    // if somehow we reach the max message size, dump the data to prevent lockup
    if (m.size == Message::maxSize)
        m.clear();

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
    // digitalWrite(STAT_PIN, HIGH);
}

// this will not work particularly well without queueing
// but it makes light go flashy if data sent so it work
void updateBlink()
{
    // if (millis() - ledTimer > g_interval && g_times > 0)
    // {
    //     ledTimer = millis();
    //     ledOn = !ledOn;
    //     digitalWrite(STAT_PIN, ledOn ? HIGH : LOW);
    //     // only decrement times after 1 cycle
    //     if (ledOn)
    //         g_times--;
    // }
    // if (g_times == 0)
    // {
    //     digitalWrite(STAT_PIN, LOW);
    // }
}

/// OLD
// Serial1.print(radHW.state);
// Serial1.print(" ");
// Serial1.print(radHW.gpio3());
// Serial1.print("\n");
// uint8_t cFIFOInfo[1] = {0b00000000};
// uint8_t rFIFOInfo[2] = {0x00, 0x00};
// radHW.sendCommand(C_FIFO_INFO, 1, cFIFOInfo, 2, rFIFOInfo);
// Serial1.println("FIFO STATUS");
// for (int i = 0; i < sizeof(rFIFOInfo); i++)
//     Serial1.println(rFIFOInfo[i]);

// if (rFIFOInfo[0] > 0)
// {
//     digitalWrite(radHW._cs, LOW);

//     // read from RX FIFO
//     radHW.spi->transfer(C_READ_RX_FIFO);

//     // holds data received this iteration
//     int lenBytes = 0;
//     int xfrd = 0;

//     // receive message data
//     int count = lenBytes;
//     while (xfrd < rFIFOInfo[0])
//     {
//         xfrd++;
//         Serial1.print(radHW.spi->transfer(0x00));
//     }
//     Serial1.println();
//     digitalWrite(radHW._cs, HIGH);
// }
// if (radHW.avail() > 0)
// {
//     Serial1.print("Got something size: ");
//     Serial1.println(radHW.avail());
//     Serial1.println(radHW.available);
//     uint16_t size = 0;
//     radHW.rx(testRXBuf, &size, sizeof(testRXBuf));
//     if (size > 0)
//     {
//         // got an actual message
//         Serial1.print("Message: ");
//         for (uint16_t i = 0; i < size; i++)
//         {
//             Serial1.print(testRXBuf[i]);
//         }
//         Serial1.print("\n");

//         // blink
//         digitalWrite(STAT_PIN, HIGH);
//         delay(200);
//         digitalWrite(STAT_PIN, LOW);
//     }
// }

// if (millis() - timer > 1000)
//     {
//         timer = millis();
//         // Serial1.println("adding msg");
//         // m.print(Serial1);
//         // Serial1.print("Success ");
//         // Serial1.println(radio.send(&m));
//         // Serial1.print("hasData ");
//         // Serial1.println(radio.txMsgs.hasData());
//     }

// for (int i = 0; i < sizeof(testTXBuf); i++)
// {
//     testTXBuf[i] = i;
// }
