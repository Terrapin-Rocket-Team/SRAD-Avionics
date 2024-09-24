#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"

APRSConfig config = {CALLSIGN, TOCALL, PATH, '[', '/'};
RadioSettings settings = {915.0, false, false, &hardware_spi, 10, 31, 32};
RFM69HCW receive = {&settings, &config};

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    if (!receive.begin())
        Serial.println("Reciever failed to begin");

    Serial.println("RFM69r began");
}

void loop()
{
    if (receive.available())
    {
        Serial.println("s");
        Serial.println(receive.receive(ENCT_GROUNDSTATION));
        Serial.println("e");
    }
}