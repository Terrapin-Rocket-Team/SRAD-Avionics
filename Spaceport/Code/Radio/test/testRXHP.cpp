#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

// radio config header
#include "422Mc80_4GFSK_009600H.h"

#define MSG_LENGTH 8160

// Si4463HardwareConfig hwcfg = {
//     MOD_4GFSK,       // modulation
//     DR_250k,         // data rate
//     (uint32_t)433e6, // frequency (Hz)
//     POWER_HP_20dBm,  // tx power (127 = ~20dBm)
//     192,             // preamble length
//     32,              // required received valid preamble
// };

// Si4463PinConfig pincfg = {
//     &SPI, // spi bus to use
//     10,   // cs
//     38,   // sdn
//     33,   // irq
//     34,   // gpio0
//     35,   // gpio1
//     36,   // gpio2
//     37,   // gpio3
// };

Si4463HardwareConfig hwcfg = {
    MOD_4GFSK,       // modulation
    DR_4_8k,         // data rate
    (uint32_t)430e6, // frequency (Hz)
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
uint16_t bufLength = 0;

uint32_t received = 0;
uint32_t timeouts = 0;
uint64_t totalLength = 0;

APRSConfig aprscfg = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};

APRSText testMessage(aprscfg);

void logStats();

void setup()
{
    Serial.begin(1000000);
    Serial5.begin(1000000);
    if (!radio.begin(CONFIG_422Mc80_4GFSK_009600H, sizeof(CONFIG_422Mc80_4GFSK_009600H)))
    // if (!radio.begin())
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
        Serial.println(radio.length);
        uint16_t receivedLength = radio.readRXBuf(buf, sizeof(buf));
        Serial.println(receivedLength);
        if (receivedLength != MSG_LENGTH)
        {
            Serial.print("Error: recevied length does not match expected length. Got: ");
            Serial.print(receivedLength);
            Serial.print(", Expected: ");
            Serial.println(MSG_LENGTH);
        }
        Serial.write(buf, receivedLength);
        totalLength += receivedLength;
        // Serial5.print("Sniffing packet: ");
        // Serial5.print(buf[0], HEX);
        // Serial5.print(" ");
        // Serial5.print(buf[1], HEX);
        // Serial5.print(" ");
        // Serial5.println(buf[2], HEX);
        radio.available = false;
        Serial5.print("Transferred: ");
        Serial5.println(totalLength);

        // reset timeout
        timer = millis();
        received++;
        // logStats();
    }
    if (millis() - timer > timeout)
    {
        timer = millis();
        timeouts++;
        Serial.println("here");
        Serial.println(radio.state);
        Serial.println(radio.readFRR(0));
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