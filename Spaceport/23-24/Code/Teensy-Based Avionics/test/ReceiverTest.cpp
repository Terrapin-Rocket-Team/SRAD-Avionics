#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"

APRSConfig confi = {CALLSIGN, TOCALL, PATH, '[', '/'};
RFM69HCW receive = {433775000, false, false, confi};

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        Serial.println("receiver loading");
    }
    SPIClass* sp = &SPI;
    receive.begin(sp, 3, 4);
    Serial.println("RFM69r began");
}

void loop() {
    if (receive.available()) {
        Serial.println(receive.receive(ENCT_TELEMETRY));
    } 
}