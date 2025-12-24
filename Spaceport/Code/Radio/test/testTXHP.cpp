#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

// #define BUZZER 0

// radio config header
#include "422Mc80_4GFSK_009600H.h"

Si4463HardwareConfig hwcfg = {
    MOD_4GFSK,        // modulation
    DR_4_8k,          // data rate
    (uint32_t)430e6,  // frequency (Hz)
    POWER_COTS_30dBm, // tx power (127 = ~20dBm)
    48,               // preamble length
    16,               // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    10,   // cs
    23,   // sdn
    20,   // irq
    21,   // gpio0
    22,   // gpio1
    36,   // gpio2
    37,   // gpio3
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();
uint32_t txTimer = micros();
bool txActive = false;

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

    if (!radio.begin(CONFIG_422Mc80_4GFSK_009600H, sizeof(CONFIG_422Mc80_4GFSK_009600H)))
    // if (!radio.begin())
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
    if (millis() - timer > 2000)
    {
        timer = millis();
        // Serial.println("Sending message");
        // Serial.println(testMessage.msg);
        txTimer = micros();
        txActive = true;
        radio.send(testMessage);
    }
    if (txActive && radio.state == STATE_IDLE)
    {
        Serial.print("TX took: ");
        Serial.println(micros() - txTimer);
        txActive = false;
    }
    // need to call as fast as possible every loop
    radio.update();
}