#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"

APRSConfig confi = {CALLSIGN, TOCALL, PATH, '[', '/'};
RFM69HCW transmit = {433775000, true, false, confi};

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        Serial.println("transmitter test");
    }
    SPIClass *sp = &SPI;
    transmit.begin(sp, 1, 2);
    Serial.println("RFM69t began");
}

void loop() {
    delay(500);
    transmit.send("39.336896667,-77.337067833,480,0,31,H,0,11:40:51", ENCT_TELEMETRY);
}