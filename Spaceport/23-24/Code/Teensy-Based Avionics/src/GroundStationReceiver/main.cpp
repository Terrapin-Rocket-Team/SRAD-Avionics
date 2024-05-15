#include "RFM69HCWNew.h"
#include <Arduino.h>

APRSHeader header;
APRSMsg msg(header);
RadioSettings settings = {915.0, false, false, &hardware_spi, 10, 31, 32};
RFM69HCWNew radio(&settings);
void setup()
{
    delay(2000);
    if(!radio.init())
        Serial.println("Radio failed to initialize");
    else
        Serial.println("Radio initialized");

    
}

void loop()
{
    if(radio.update())
    {
        if(radio.dequeueReceive(&msg))
        {
            printf("Received message (%d):", radio.RSSI());
            printf("\nHeader: %s>%s,%s:\n", msg.header.CALLSIGN, msg.header.TOCALL, msg.header.PATH);
            printf("Data: %f, %f, %f, %f, %f, %d, %f, %f, %f\n", msg.data.lat, msg.data.lng, msg.data.alt, msg.data.spd, msg.data.hdg, msg.data.stage, msg.data.orientation.x(), msg.data.orientation.y(), msg.data.orientation.z());
        }
    }
}