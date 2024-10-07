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
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};

APRSText testMessage(aprscfg);

void setup()
{
    Serial.begin(9600);
    if (!radio.begin())
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
        radio.receive(testMessage);
        Serial.println(testMessage.msg);
    }
    // need to call as fast as possible every loop
    radio.update();
}