#include <Arduino.h>
#include <MMFS.h>

// Helper function to generate a random matrix 
mmfs::Matrix randomMatrix(int rows, int cols) {
    double *arr = new double[rows * cols];
    for (int i = 0; i < rows * cols; i++) {
        arr[i] = random(1, 10000);  // Random values between 1 and 10000
    }
    return mmfs::Matrix(rows, cols, arr);
}

// Benchmark function
void benchmark_inverse(int matrix_size, int iterations) {
    unsigned long startTime, endTime;
    unsigned long totalDuration = 0;

    for (int i = 0; i < iterations; i++) {
        mmfs::Matrix mat = randomMatrix(matrix_size, matrix_size);
        
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

// Arduino setup function
void setup() {
    Serial.begin(115200);  
    delay(2000); 

    // Run benchmarks for different matrix sizes
    for (int i = 2; i <= 30; i++) {
        benchmark_inverse(i, 1000);
    }
}

void loop() {
    // Nothing to do here
}


/**
 * Output:
Average time for matrix size 2x2 = 2 microseconds
Average time for matrix size 3x3 = 4 microseconds
Average time for matrix size 4x4 = 6 microseconds
Average time for matrix size 5x5 = 10 microseconds
Average time for matrix size 6x6 = 15 microseconds
Average time for matrix size 7x7 = 22 microseconds
Average time for matrix size 8x8 = 31 microseconds
Average time for matrix size 9x9 = 42 microseconds
Average time for matrix size 10x10 = 55 microseconds
Average time for matrix size 11x11 = 70 microseconds
Average time for matrix size 12x12 = 88 microseconds
Average time for matrix size 13x13 = 110 microseconds
Average time for matrix size 14x14 = 134 microseconds
Average time for matrix size 15x15 = 162 microseconds
Average time for matrix size 16x16 = 194 microseconds
Average time for matrix size 17x17 = 229 microseconds
Average time for matrix size 18x18 = 269 microseconds
Average time for matrix size 19x19 = 313 microseconds
Average time for matrix size 20x20 = 361 microseconds
Average time for matrix size 21x21 = 414 microseconds
Average time for matrix size 22x22 = 472 microseconds
Average time for matrix size 23x23 = 536 microseconds
Average time for matrix size 24x24 = 605 microseconds
Average time for matrix size 25x25 = 680 microseconds
Average time for matrix size 26x26 = 760 microseconds
Average time for matrix size 27x27 = 847 microseconds
Average time for matrix size 28x28 = 939 microseconds
Average time for matrix size 29x29 = 1039 microseconds
Average time for matrix size 30x30 = 1145 microseconds
*/