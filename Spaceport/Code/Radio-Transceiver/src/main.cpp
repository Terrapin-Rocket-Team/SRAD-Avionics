#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"

APRSConfig config = {CALLSIGN, TOCALL, PATH, '[', '/'};
RadioSettings settings = {433.000, true, false, &hardware_spi, 10, 2, 9};
RFM69HCW transmit = {&settings, &config};
APRSMsg msg = APRSMsg();
void setup()
{
    Serial.begin(9600);
    Serial1.begin(115200);
    // if (!transmit.begin())
    //     Serial.println("Transmitter failed to begin");

    Serial.println("RFM69t began");
}

void loop()
{
    if(Serial1.available())
    {
        String message = Serial1.readStringUntil('\n');
        bool b = msg.decode(message.c_str());
        if(b)
            {
                Serial.println("Message decoded");
                char *message = new char[100];
                msg.toString(message);
                Serial.println(message);
            }
            else{
                Serial.println("Message not decoded");
                Serial.println(message);
            }
    }
}