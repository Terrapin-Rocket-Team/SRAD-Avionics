#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"

APRSConfig config = {CALLSIGN, TOCALL, PATH, '[', '/'};
// RadioSettings settingsT = {433.775, true, false, &hardware_spi, 10, 2, 9};
RadioSettings settingsR = {433.775, false, false, &hardware_spi, 10, 31, 32};
// RFM69HCW transmit = {settingsT, config};
RFM69HCW receive = {settingsR, config};
// uint32_t timer = millis();

// char radiopacket[150] = "39.336896667,-77.337067833,480,0,31,H,0,11:40:51";

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;

    // if (!transmit.begin())
    //     Serial.println("Transmitter failed to begin");
    if (!receive.begin())
        Serial.println("Reciever failed to begin");

    // Serial.println("RFM69 began");
    pinMode(33, OUTPUT);
}

void loop()
{
    if (receive.available())
    {
        // digitalWrite(33, HIGH);
        // delay(100);
        // digitalWrite(33, LOW);
        // Serial.print("Received: ");
        Serial.println("s");
        Serial.println(receive.receive(ENCT_GROUNDSTATION));
        Serial.println("e");
    }

    // if (millis() - timer >= 1000)
    // {
    //     timer = millis();
    //     Serial.println("Sending...");
    //     transmit.send(radiopacket, ENCT_TELEMETRY);
    // }
}