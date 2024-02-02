#include <Arduino.h>
#include "AvionicsState.cpp"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "Storage/recordData.h"

BNO055 bno(13, 12);   //I2C Address 0x29
BMP390 bmp(13, 12);   //I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); //I2C Address 0x42  
DS3231 rtc();   //I2C Address 0x68

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