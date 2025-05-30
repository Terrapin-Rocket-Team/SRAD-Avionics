#include "Arduino.h"
#include "RadioMessage.h"
#include "MockRadio.h"

#define MSG_LENGTH 8160

MockHardwareConfig hwcfg = {
    500000};

MockPinConfig pincfg = {
    &Serial1};

MockRadio radio(hwcfg, pincfg);
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
        uint16_t receivedLength = radio.readRXBuf(buf, radio.length);
        // if (receivedLength != MSG_LENGTH)
        // {
        //     Serial.print("Error: recevied length does not match expected length. Got: ");
        //     Serial.print(receivedLength);
        //     Serial.print(", Expected: ");
        //     Serial.println(MSG_LENGTH);
        // }
        Serial.write(buf, receivedLength);
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