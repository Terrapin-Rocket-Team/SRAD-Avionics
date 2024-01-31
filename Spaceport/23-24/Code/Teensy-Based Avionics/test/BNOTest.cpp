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
    Serial.println(bno.getStaticDataString());
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
    Serial.print(bno.get_orientation_euler().x());
    Serial.print(", ");
    Serial.print(bno.get_orientation_euler().y());
    Serial.print(", ");
    Serial.println(bno.get_orientation_euler().z());

}