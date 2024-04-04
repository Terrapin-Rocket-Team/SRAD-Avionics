#include <Arduino.h>
#include "APRSMsg.h"
#include "RFM69HCW.h"
#include "Radio.h"

#define BUZZER 33

APRSConfig config = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
RadioSettings settings1 = {433.775, false, false, &hardware_spi, 10, 31, 32};
RadioSettings settings2 = {915.775, false, false, &hardware_spi, 10, 31, 32};       //change pins
RadioSettings settings3 = {915.555, false, false, &hardware_spi, 10, 31, 32};       //change pins 
RFM69HCW radio1 = {settings1, config};
RFM69HCW radio2 = {settings2, config};
RFM69HCW radio3 = {settings3, config};

int lastCycle = -1;
RFM69HCW *curSystem = &radio1;

void setup() {

    Serial.begin(115200);      // 921600 baud rate (https://lucidar.me/en/serialib/most-used-baud-rates-table/)

    radio1.begin();
    radio2.begin();
    radio3.begin();

    lastCycle = Serial.read();  
    pinMode(BUZZER, OUTPUT);

    digitalWrite(BUZZER, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
    delay(1000);

}

void loop() {

    int phase = Serial.read();

    if (phase != -1) {
        lastCycle = phase;

        if (phase == 1) {
            curSystem = &radio1;
            // digitalWrite(BUZZER, HIGH);
            // delay(200);
            // digitalWrite(BUZZER, LOW);
            // delay(100);
        } else if (phase == 2) {
            curSystem = &radio2;
            // digitalWrite(BUZZER, HIGH);
            // delay(1000);
            // digitalWrite(BUZZER, LOW);
            // delay(100);
        } else {
            curSystem = &radio3;
            // digitalWrite(BUZZER, HIGH);
            // delay(100);
            // digitalWrite(BUZZER, LOW);
            // delay(100);
            // digitalWrite(BUZZER, HIGH);
            // delay(100);
            // digitalWrite(BUZZER, LOW);
            // delay(100);
        }
    }
    // else {
        // digitalWrite(BUZZER, HIGH);
        // delay(200);
        // digitalWrite(BUZZER, LOW);
        // delay(200);
        // delay(50);
    // }

    if (lastCycle == 1) {
        Serial.print("a");
    }
    else if (lastCycle == 2) {
        Serial.print("b");
    }
    else if (lastCycle == 3) {
        Serial.print("c");
    }
    else {
        Serial.print("Invalid phase");
    }

    // need this so that it sends even if it doesn't receive
    // if (curSystem->availableX()) {
    //     if (lastCycle == 1) {
    //         const char *msg = curSystem->receiveX(ENCT_GROUNDSTATION);
    //         if (strcmp(msg, "") != 0) {
    //             Serial.print(msg);
    //         }
    //     } 
    //     else if (lastCycle == 2 || lastCycle == 3) {
    //         const char *msg = curSystem->rxX();

    //         if (strcmp(msg, "Failed to receive message") != 0 && strcmp(msg, "No message available") != 0) {
    //             Serial.print(msg);
    //         }
    //     }
    //     else {
    //         Serial.print("Invalid phase");
    //         // delay(100);
    //     }
    // }




}
