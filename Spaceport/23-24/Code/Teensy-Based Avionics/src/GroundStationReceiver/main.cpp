#include "RFM69HCWNew.h"
#include <Arduino.h>

APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1"};
APRSMsg msg(header);
RadioSettings settings = {915.0, false, false, &hardware_spi, 10, 31, 32};
RFM69HCWNew radio(&settings);
void setup()
{
    if(!radio.init())
        Serial.println("Radio failed to initialize");
    else
        Serial.println("Radio initialized");

    
}

void loop()
{
    if(radio.update())
    {
        APRSMsg *m;
        if(radio.dequeueReceive(m))
        {
            Serial.println("Received message:");
            printf("Header: %s>%s,%s\n", m->header.CALLSIGN, m->header.TOCALL, m->header.PATH);
            printf("Data: lat: %f, lng: %f, alt: %f, spd: %f, hdg: %f, stage: %d, orientation: %f %f %f\n", m->data.lat, m->data.lng, m->data.alt, m->data.spd, m->data.hdg, m->data.stage, m->data.orientation[0], m->data.orientation[1], m->data.orientation[2]);
        }
    }
}