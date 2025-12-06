#include "Arduino.h"
#include "RadioMessage.h"
#include "MockRadio.h"

// #define BUZZER 0

// radio config header
#include "422Mc110_2GFSK_500000U.h"

MockHardwareConfig hwcfg = {
    500000};

MockPinConfig pincfg = {
    (HardwareSerial *)&Serial1};

MockRadio radio(hwcfg, pincfg);
uint32_t timer = millis();

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

APRSText testMessage(aprscfg, "RSSI test, longer test message", "");

// void beep(int d)
// {
//     digitalWrite(BUZZER, HIGH);
//     delay(d);
//     digitalWrite(BUZZER, LOW);
//     delay(d);
// }

void setup()
{
    Serial.begin(9600);
    // pinMode(BUZZER, OUTPUT);
    // digitalWrite(BUZZER, LOW);

    if (!radio.begin())
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
        {
            // beep(1000);
        }
    }
    Serial.println("Radio began successfully");

    // beep(100);
}

void loop()
{
    if (millis() - timer > 1)
    {
        timer = millis();
        Serial.println("Sending message");
        Serial.println(testMessage.msg);
        radio.send(testMessage);
    }
    // need to call as fast as possible every loop
    radio.update();
}