#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

// radio config header
#include "422Mc110_2GFSK_500000U.h"

#define MSG_LENGTH 8160

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK,       // modulation
    DR_500k,         // data rate
    (uint32_t)433e6, // frequency (Hz)
    POWER_HP_20dBm,  // tx power (127 = ~20dBm)
    48,              // preamble length
    16,              // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    30,   // cs
    29,   // sdn
    24,   // irq
    25,   // gpio0
    26,   // gpio1
    27,   // gpio2
    28,   // gpio3
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();
uint32_t timeout = 2100;
uint8_t buf[MSG_LENGTH];

uint32_t received = 0;
uint32_t timeouts = 0;

APRSConfig aprscfg = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};

APRSText testMessage(aprscfg);

void logStats();

void setup()
{
    Serial.begin(1000000);
    if (!radio.begin(CONFIG_422Mc110_2GFSK_500000U, sizeof(CONFIG_422Mc110_2GFSK_500000U)))
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
            ;
    }
    Serial.println("Radio began successfully");
}

void loop()
{
    if (radio.avail())
    {
        // Serial.println("here");
        uint16_t receivedLength = radio.readRXBuf(buf, sizeof(buf));
        if (receivedLength != MSG_LENGTH)
        {
            Serial.print("Error: recevied length does not match expected length. Got: ");
            Serial.print(receivedLength);
            Serial.print(", Expected: ");
            Serial.println(MSG_LENGTH);
        }
        // Serial.write(buf, receivedLength);
        Serial.print("Sniffing packet: ");
        Serial.print(buf[0], HEX);
        Serial.print(buf[1], HEX);
        Serial.println(buf[2], HEX);
        radio.available = false;

        // reset timeout
        timer = millis();
        received++;
        // logStats();
    }
    if (millis() - timer > timeout)
    {
        timer = millis();
        timeouts++;
        // logStats();
    }
    // need to call as fast as possible every loop
    radio.update();
}

// void logStats()
// {
//     Serial.print("Received: ");
//     Serial.print(received);
//     Serial.print(" | Timeouts: ");
//     Serial.println(timeouts);
// }