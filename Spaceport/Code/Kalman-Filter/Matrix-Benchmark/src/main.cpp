#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

void benchmark_inverse(int matrix_size, int iterations) {
    unsigned long startTime, endTime;
    unsigned long totalDuration = 0;

    for (int i = 0; i < iterations; i++) {
        Matrix mat = Matrix::random(matrix_size, matrix_size);  // Assuming random matrix generator
        startTime = micros();
        mat.inverse();
        endTime = micros();
        totalDuration += (endTime - startTime);
    }

    Serial.print("Average time for matrix size ");
    Serial.print(matrix_size);
    Serial.print("x");
    Serial.print(matrix_size);
    Serial.print(" = ");
    Serial.print(totalDuration / iterations);
    Serial.println(" microseconds");
}
