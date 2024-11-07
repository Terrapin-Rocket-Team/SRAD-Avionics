/*
 * Project: Si4463 Radio Library for AVR and Arduino (Ping server example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Ping server
 *
 * Listen for packets and send them back
 */

#include <Si446x.h>
#include <Si4463.h>

#define CHANNEL 0
#define MAX_PACKET_SIZE 10

#define PACKET_NONE 0
#define PACKET_OK 1
#define PACKET_INVALID 2

Si4463HardwareConfig hwcfg = {
    MOD_2FSK, // modulation
    DR_40k,   // data rate
    433e6,    // frequency (Hz)
    127,      // tx power (127 = ~20dBm)
    48,       // preamble length
    16,       // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);

typedef struct
{
    uint8_t ready;
    int16_t rssi;
    uint8_t length;
    uint8_t buffer[MAX_PACKET_SIZE];
} pingInfo_t;

static volatile pingInfo_t pingInfo;

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
    if (length > MAX_PACKET_SIZE)
        length = MAX_PACKET_SIZE;

    pingInfo.ready = PACKET_OK;
    pingInfo.rssi = rssi;
    pingInfo.length = length;

    Si446x_read((uint8_t *)pingInfo.buffer, length);

    // Radio will now be in idle mode
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
    pingInfo.ready = PACKET_INVALID;
    pingInfo.rssi = rssi;
}

void setup()
{
    Serial.begin(9600);

    // pinMode(A5, OUTPUT); // LED

    Serial.println("Starting Setup");

    radio.begin();

    // Start up
    Si446x_init();
    Si446x_setTxPower(SI446X_MAX_TX_POWER);
    Serial.println("Setup");

    // Put into receive mode
    // Si446x_RX(CHANNEL);
    radio.rx();

    Serial.println(F("Waiting for ping..."));
}

uint32_t timer = millis();

void loop()
{
    static uint32_t pings;
    static uint32_t invalids;

    // Wait for data
    if (radio.avail())
    {
        radio.available = false;

        uint8_t message[radio.maxLen] = {0};
        uint16_t messageLen = radio.length;
        memcpy(message, radio.buf, radio.length);

        // if (pingInfo.ready != PACKET_OK)
        // {
        //     invalids++;
        //     pingInfo.ready = PACKET_NONE;
        //     Serial.print(F("Invalid packet! Signal: "));
        //     Serial.print(pingInfo.rssi);
        //     Serial.println(F("dBm"));
        //     Si446x_RX(CHANNEL);
        // }
        // else
        // {
        pings++;

        Serial.println(F("Got ping, sending reply..."));

        // Send back the data, once the transmission has completed go into receive mode
        radio.tx(message, messageLen);

        Serial.println(F("Reply sent"));

        // Toggle LED
        // static uint8_t ledState;
        // digitalWrite(A5, ledState ? HIGH : LOW);
        // ledState = !ledState;

        Serial.print(F("Signal strength: "));
        Serial.print(radio.RSSI());
        Serial.println(F("dBm"));

        // Print out ping contents
        Serial.print(F("Data from server: "));
        Serial.write(message, messageLen);
        Serial.println();
        // }

        Serial.print(F("Totals: "));
        Serial.print(pings);
        Serial.print(F("Pings, "));
        Serial.print(invalids);
        Serial.println(F("Invalid"));
        Serial.println(F("------"));

        // Put into receive mode
        // Si446x_RX(CHANNEL);
        // radio.rx();

        Serial.println(F("Waiting for ping..."));
        Serial.println();
    }

    radio.update();
}
