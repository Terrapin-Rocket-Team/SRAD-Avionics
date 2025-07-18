#include "Arduino.h"
// #include "RadioMessage.h"
// #include "Si4463.h"

// #define BUZZER 0

// Si4463HardwareConfig hwcfg = {
//     MOD_2GFSK, // modulation
//     DR_100k,   // data rate
//     433e6,     // frequency (Hz)
//     127,       // tx power (127 = ~20dBm)
//     48,        // preamble length
//     16,        // required received valid preamble
// };

// Si4463PinConfig pincfg = {
//     &SPI, // spi bus to use
//     8,    // cs
//     6,    // sdn
//     7,    // irq
//     9,    // gpio0
//     10,   // gpio1
//     4,    // random pin - gpio2 is not connected
//     5,    // random pin - gpio3 is not connected
// };

// Si4463 radio(hwcfg, pincfg);
// uint32_t timer = millis();

// APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

// APRSText testMessage(aprscfg, "RSSI test, longer test message", "");

// void beep(int d)
// {
//     digitalWrite(BUZZER, HIGH);
//     delay(d);
//     digitalWrite(BUZZER, LOW);
//     delay(d);
// }

void setup()
{
    Serial.begin(500000);
    Serial1.begin(500000);
}

void loop()
{
    if (Serial1.available())
        Serial.write(Serial1.read());
}