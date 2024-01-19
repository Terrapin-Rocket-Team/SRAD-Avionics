#include <Arduino.h>
#include "RFM69HCW.h"

#define CALLSIGN "KC3UTM"
#define TOCALL "APRS"
#define PATH "WIDE1-1"
#define SYMBOL "["
#define OVERLAY "/"

APRSConfig confi = {CALLSIGN, TOCALL, PATH, SYMBOL, OVERLAY};
RFM69HCW receive = {433775000, false, false, confi};

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        Serial.println("receiver test");
    }
    SPIClass* spi;
    receive.begin(spi, 3, 4, 433);
    Serial.println("RFM69r began");
}

void loop() {
    delay(1000);
    if (receive.available()) {
            Serial.println(receive.receive(0));
    } else {
        Serial.println("not available");
    }
}