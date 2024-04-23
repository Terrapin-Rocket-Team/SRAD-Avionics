#include <iostream>
#include <windows.h>
#include "../Filters/LinearKalmanFilter/LinearKalmanFilter.h"

void printMatrix(double* matrix, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            std::cout << matrix[i * cols + j] << "\t";
        }
        std::cout << std::endl;
    }
}


void printState(const KFState& state) {
    std::cout << "X:" << std::endl;
    printMatrix(state.X, 6, 1);
    std::cout << std::endl;
}

int main() {
    // Initialize initial_state and initial_control arrays
    double* initial_state = new double[6]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double* initial_control = new double[3]{0.0, 0.0, 0.0};
    printf("yoooo");
    // Call initialize function
    KFState state = initialize(6, 3, 3, initial_state, initial_control);
    printf("hii");
    // Print the initialized state
    printState(state);
    double* measurement = new double[3]{0, 0, 0};
    while (1){
        state = iterate(state, 0.05, measurement, initial_control, 0);
        printState(state);
    }
    // Clean up
    delete[] state.X;
    delete[] state.U;
    delete[] state.P;

    return 0;
}
