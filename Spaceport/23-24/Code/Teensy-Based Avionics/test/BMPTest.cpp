// #include <Arduino.h>
// #include "BMP390.h"

// BMP390 bmp(13, 12);     // put pins here

// void setup() {
//     Serial.begin(9600);
//     while (!Serial);
//     Serial.println("BMP390 test");
    
//     bmp.initialize();
//     Serial.println("BMP390 initialized");
//     delay(1000);
//     char* str = bmp.getStaticDataString();
//     Serial.println(str);
//     delete[] str;
// }

// void loop() {
//     delay(200);
//     Serial.print("Pressure: ");
//     Serial.print(bmp.get_pressure());

//     Serial.print(", Temperature: ");
//     Serial.print(bmp.get_temp());

//     Serial.print(", Altitude: ");
//     Serial.println(bmp.get_rel_alt_ft());
// }