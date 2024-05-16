#include "RFM69HCW.h"
#include "EncodeAPRSForSerial.h"
#include "Adafruit_BNO055.h"
#include <Arduino.h>

APRSHeader header;
APRSMsg msg(header);
RadioSettings settings = {915.0, false, false, &hardware_spi, 10, 31, 32};
RFM69HCW radio(&settings);
void setup()
{
    delay(2000);
    if (!radio.init())
        Serial.println("Radio failed to initialize");
    else
        Serial.println("Radio initialized");
}

void loop()
{
    if (radio.update())
    {
        if (radio.dequeueReceive(&msg))
        {
            char buffer[255];
            aprsToSerial::encodeAPRSForSerial(msg, buffer, 255, radio.RSSI());
            Serial.println(buffer);
        }
    }
}