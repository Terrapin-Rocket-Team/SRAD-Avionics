#include <Arduino.h>

int data = 0;

void setup() {
  pinMode(2, INPUT); // IRQ Pin on Si4463
  Serial1.begin(9600);
}

void loop() {
  while (Serial1.available() > 0) {
    data = Serial1.read();
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
