#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

// radio config header
#include "422Mc86_4GFSK_500000H.h"

#define BUZZER 0

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK,       // modulation
    DR_500k,         // data rate
    (uint32_t)433e6, // frequency (Hz)
    127,             // tx power (127 = ~20dBm)
    48,              // preamble length
    16,              // required received valid preamble
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
uint32_t throttleTimer = millis();

Message m;

uint16_t buf1Size = 50;
uint16_t buf2Size = 50;
uint16_t buf3Size = 50;
uint16_t buf4Size = 50;

uint8_t buf1[50] = {0};
uint8_t buf2[50] = {0};
uint8_t buf3[50] = {0};
uint8_t buf4[50] = {0};

int counter = 3;

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

APRSText testMessage(aprscfg, "test with payload longer than FIFO length, test with payload longer than FIFO length, test with payload longer than FIFO length", "");

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

    if (!radio.begin(CONFIG_422Mc86_4GFSK_500000H, sizeof(CONFIG_422Mc86_4GFSK_500000H)))
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
        {
            beep(1000);
        }
    }
    Serial.println("Radio began successfully");

    m.encode(&testMessage);

    m.shift(buf1, buf1Size);
    m.shift(buf2, buf2Size);
    m.shift(buf3, buf3Size);
    m.shift(buf4, buf4Size);

    beep(100);
}

void loop()
{
    if (counter < 3 && millis() - throttleTimer > 2)
    {
        throttleTimer = millis();
        if (counter == 0)
        {
            uint16_t bufSize = buf2Size;
            radio.writeTXBuf(buf2, bufSize);
        }
        if (counter == 1)
        {
            uint16_t bufSize = buf3Size;
            radio.writeTXBuf(buf3, bufSize);
        }
        if (counter == 2)
        {
            uint16_t bufSize = buf4Size;
            radio.writeTXBuf(buf4, bufSize);
        }
        counter++;
    }
    if (millis() - timer > 2000)
    {
        timer = millis();
        counter = 0;
        Serial.println("Sending message");
        Serial.println(testMessage.msg);
        // radio.send(testMessage);
        Serial.println(buf1Size + buf2Size + buf3Size + buf4Size);
        radio.startTX(buf1, buf1Size, buf1Size + buf2Size + buf3Size + buf4Size);
    }
    // need to call as fast as possible every loop
    radio.update();
}