/*
 * Project: Si4463 Radio Library for AVR and Arduino (Ping client example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Ping client
 *
 * Time how long it takes to send some data and get a reply
 * Should be around 5-6ms with default settings
 */

#include <Si446x.h>
#include <Si4463.h>

#define BUZZER 0

#define CHANNEL 0
#define MAX_PACKET_SIZE 10
#define TIMEOUT 1000

#define PACKET_NONE 0
#define PACKET_OK 1
#define PACKET_INVALID 2

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_40k,    // data rate
    433e6,     // frequency (Hz)
    127,       // tx power (127 = ~20dBm)
    48,        // preamble length
    16,        // required received valid preamble
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
    uint32_t timestamp;
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
    pingInfo.timestamp = millis();
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

void beep(int d)
{
    // digitalWrite(BUZZER, HIGH);
    delay(d);
    digitalWrite(BUZZER, LOW);
    delay(d);
}

void setup()
{
    Serial.begin(9600);

    pinMode(BUZZER, OUTPUT);

    // pinMode(A5, OUTPUT); // LED

    Serial.println("Starting Setup");

    radio.begin();

    // Start up
    Si446x_init();
    Si446x_setTxPower(SI446X_MAX_TX_POWER);
    Serial.println("Setup");

    beep(500);
}

uint32_t timer = millis();

void loop()
{
    static uint8_t counter;
    static uint32_t sent;
    static uint32_t replies;
    static uint32_t timeouts;
    static uint32_t invalids;

    if (millis() - timer > 1500)
    {
        timer = millis();
        // Make data
        char data[MAX_PACKET_SIZE] = {0};
        sprintf_P(data, PSTR("test %hhu"), counter);
        counter++;

        Serial.print(F("Sending data: "));
        Serial.println(data);

        beep(100);
        uint32_t startTime = millis();

        // Send the data
        // Si446x_TX(data, sizeof(data), CHANNEL, SI446X_STATE_RX);
        radio.tx((const uint8_t *)data, sizeof(data));
        sent++;

        Serial.println(F("Data sent, waiting for reply..."));

        uint8_t success;

        // Wait for reply with timeout
        uint32_t sendStartTime = millis();
        while (1)
        {
            if (radio.avail())
                break;

            else if (millis() - sendStartTime > TIMEOUT) // Timeout // TODO typecast to uint16_t
                break;
        }
        uint32_t endTime = millis();

        // pingInfo.ready = PACKET_NONE;

        if (!radio.available)
        {
            Serial.println(F("Ping timed out"));
            timeouts++;
        }
        // else if (success == PACKET_INVALID)
        // {
        //     Serial.print(F("Invalid packet! Signal: "));
        //     Serial.print(pingInfo.rssi);
        //     Serial.println(F("dBm"));
        //     invalids++;
        // }
        else
        {
            radio.available = false;

            uint8_t message[radio.maxLen] = {0};
            uint16_t messageLen = radio.length;
            memcpy(message, radio.buf, radio.length);
            // If success toggle LED and send ping time over UART
            uint16_t totalTime = endTime - startTime;

            // static uint8_t ledState;
            // digitalWrite(A5, ledState ? HIGH : LOW);
            // ledState = !ledState;

            replies++;

            Serial.print(F("Ping time: "));
            Serial.print(totalTime);
            Serial.println(F("ms"));

            Serial.print(F("Signal strength: "));
            Serial.print(radio.RSSI());
            Serial.println(F("dBm"));

            // Print out ping contents
            Serial.print(F("Data from server: "));
            Serial.write(message, messageLen);
            Serial.println();
        }

        Serial.print(F("Totals: "));
        Serial.print(sent);
        Serial.print(F(" Sent, "));
        Serial.print(replies);
        Serial.print(F(" Replies, "));
        Serial.print(timeouts);
        Serial.print(F(" Timeouts, "));
        Serial.print(invalids);
        Serial.println(F(" Invalid"));
        Serial.println(F("------"));
    }
    radio.update();
}
