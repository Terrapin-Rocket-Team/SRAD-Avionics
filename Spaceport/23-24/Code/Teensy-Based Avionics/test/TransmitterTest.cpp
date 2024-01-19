#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"
#define SYMBOL "["
#define OVERLAY "/"

APRSConfig confi = {CALLSIGN, TOCALL, PATH, SYMBOL, OVERLAY};
RFM69HCW transmit = {433775000, true, false, confi};

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        Serial.println("transmitter test");
    }
    SPIClass* spi;
    transmit.begin(spi, 1, 2, 433);
    Serial.println("RFM69t began");
}

void loop() {
    delay(1000);
    transmit.send("sending a message", 0);
}