#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

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
    10,   // cs
    38,   // sdn
    33,   // irq
    34,   // gpio0
    35,   // gpio1
    36,   // random pin - gpio2 is not connected
    37,   // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();
uint32_t timeout = 2100;

uint32_t received = 0;
uint32_t timeouts = 0;

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};

APRSText testMessage(aprscfg);

void logStats();

void setup()
{
    Serial.begin(9600);
    Serial.println("START");
    if (!radio.begin())
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        delay(1000);
        while (1)
            ;
    }
    Serial.println("Radio began successfully");
}

void loop()
{
    if (radio.avail())
    {
        radio.receive(testMessage);
        Serial.print("\nReceived message: ");
        Serial.println(testMessage.msg);
        Serial.print("RSSI: ");
        Serial.print(radio.RSSI());
        Serial.println(" dBm");

        // reset timeout
        timer = millis();
        received++;
        logStats();
    }
    if (millis() - timer > timeout)
    {
        timer = millis();
        timeouts++;
        logStats();
    }
    // need to call as fast as possible every loop
    radio.update();
}

void logStats()
{
    Serial.print("Received: ");
    Serial.print(received);
    Serial.print(" | Timeouts: ");
    Serial.println(timeouts);
}