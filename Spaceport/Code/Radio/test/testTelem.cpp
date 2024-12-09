#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

#define BUZZER 0

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

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

char msg[Message::maxSize + 1];
uint16_t pos = 0;
bool msgComplete = false;

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
    Serial5.begin(115200);
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

    memset(msg, 0, sizeof(msg));

    beep(100);
}

void loop()
{
    if (Serial5.available() > 0 && !msgComplete && pos < Message::maxSize)
    {
        char c = Serial5.
        Serial.write(c);
        if (c == '\n')
            msgComplete = true;
        if (pos < Message::maxSize && c != '\0' && c != '\n')
        {
            msg[pos++] = c;
        }
    }
    if (msgComplete)
    {
        timer = millis();
        Serial.println("Sending message");
        // Serial.println(msg);
        radio.tx((uint8_t *)msg, pos);
        memset(msg, 0, sizeof(msg));
        pos = 0;
        msgComplete = false;
    }
    // need to call as fast as possible every loop
    radio.update();
}