#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

#define BUZZER 0

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_500b,   // data rate
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

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

APRSText testMessage(aprscfg, "RSSI test, longer test message", "");

void beep(int d)
{
    digitalWrite(BUZZER, HIGH);
    delay(d);
    digitalWrite(BUZZER, LOW);
    delay(d);
}

void setup()
{
    Serial.begin(9600);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    if (!radio.begin())
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
        {
            beep(1000);
        }
    }
    Serial.println("Radio began successfully");

    beep(100);
}

void loop()
{
    if (millis() - timer > 2000)
    {
        timer = millis();
        Serial.println("Sending message");
        Serial.println(testMessage.msg);
        radio.send(testMessage);
    }
    // need to call as fast as possible every loop
    radio.update();
}