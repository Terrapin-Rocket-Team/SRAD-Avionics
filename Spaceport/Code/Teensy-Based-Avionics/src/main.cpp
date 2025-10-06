

#include <Arduino.h>


void setup() {
    Serial1.begin(115200);
    Serial.begin(9600);
}
double lastT = 0, lastL = 0;
int count = 0, countL = 0;
void loop() {
    if (Serial1.available()) {
        Serial.write(Serial1.read());
    }
    if (Serial.available()) {
        Serial1.write(Serial.read());
    }

    // if(millis() - lastT > 20) {
    //     lastT = millis();
    //     Serial1.printf("TELEM/%d,%f,0,1000,900,abcdefghijklmnopqrstuvwxyz\n", count++, lastT/1000.0);
    //     Serial.printf("TELEM/%d,%f,0,1000,900,abcdefghijklmnopqrstuvwxyz\n", count++, lastT/1000.0);
    // }
    // if(millis() - lastL > 5000) {
    //     lastL = millis();
    //     Serial1.printf("LOG/%f [INFO] This is the %dth log message.\n", lastL/1000.0, countL++);
    //     Serial.printf("LOG/%f [INFO] This is the %dth log message.\n", lastL/1000.0, countL++);
    // }
}