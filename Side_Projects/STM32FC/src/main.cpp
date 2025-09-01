#include <Arduino.h>

void setup() {
  // Configure PB11 as output
  pinMode(PB11, OUTPUT);
}

void loop() {
  // Toggle PB11 every 500 ms
  digitalWrite(PB11, HIGH);
  delay(500);
  digitalWrite(PB11, LOW);
  delay(500);
}
