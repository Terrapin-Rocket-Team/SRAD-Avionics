#include <Arduino.h>
#include "BNO055.h"

BNO055 bno(13, 12);     

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("BNO055 test");
    
    bno.initialize();
    Serial.println("BNO055 initialized");
    delay(1000);
    char* str = bno.getStaticDataString();
    Serial.println(str);
    delete[] str;
}

void loop() {
    delay(200);
    Serial.print("Acceleration: ");
    Serial.print(bno.get_acceleration().x());
    Serial.print(", ");
    Serial.print(bno.get_acceleration().y());
    Serial.print(", ");
    Serial.println(bno.get_acceleration().z());

    Serial.print("Orientation: ");
    Serial.print(bno.getOrientationEuler().x());
    Serial.print(", ");
    Serial.print(bno.getOrientationEuler().y());
    Serial.print(", ");
    Serial.println(bno.getOrientationEuler().z());

}