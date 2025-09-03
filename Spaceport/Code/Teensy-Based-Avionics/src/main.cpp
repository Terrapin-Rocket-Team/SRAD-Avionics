#include <Arduino.h>

void setup(){
    Serial1.begin(115200);
}

void loop(){
    if(Serial1.available()){
        Serial.println(Serial1.readStringUntil('\n'));
    }
}