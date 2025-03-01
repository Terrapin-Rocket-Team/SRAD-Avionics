#include <Arduino.h>
#include <SPI.h>

// Si4463 Pin Assignments
#define Si_IRQ 0

void Si_IRQ_handler() {
  
}

void setup() {
  Serial1.begin(9600);
  SPI.begin();  // Teensy SPI bus 0
  SPI1.begin(); // Teensy SPI bus 1
  pinMode(Si_IRQ, INPUT_PULLUP);

}

void loop() {
  while (Serial1.available()) {
    
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
