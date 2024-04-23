#include <iostream>
#include "../MatrixMult/MatrixMult.h"

void printMatrix(double* matrix, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            std::cout << matrix[i * cols + j] << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    const int size = 5;
    double matrix1[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    double matrix2[size * size] = {
        25, 24, 23, 22, 21,
        20, 19, 18, 17, 16,
        15, 14, 13, 12, 11,
        10, 9, 8, 7, 6,
        5, 4, 3, 2, 1
    };

    std::cout << "Matrix 1:" << std::endl;
    printMatrix(matrix1, 3, 3);
    std::cout << std::endl;

    std::cout << "Matrix 2:" << std::endl;
    printMatrix(matrix2, size, size);
    std::cout << std::endl;

    // Test multiply_matrices
    // double* mult_result = multiplyMatrices(matrix1, matrix2, size, size, size, size);
    // std::cout << "Matrix Multiplication Result:" << std::endl;
    // printMatrix(mult_result, size, size);
    // std::cout << std::endl;

    // // Test inverseMatrix
    double* inv_result = inverseMatrix(matrix1, 3);
    if (inv_result != nullptr) {
        std::cout << "Inverse of Matrix 1asd:" << std::endl;
        printMatrix(inv_result, 3, 3);
        std::cout << std::endl;
    }

    // Test transposeMatrix
    // double* transpose_result = transposeMatrix(matrix1, size, size);
    // std::cout << "Transpose of Matrix 1:" << std::endl;
    // printMatrix(transpose_result, size, size);
    // std::cout << std::endl;

    // Test addMatrices
    // double* add_result = addMatrices(matrix1, matrix2, size, size);
    // std::cout << "Matrix Addition Result:" << std::endl;
    // printMatrix(add_result, size, size);
    // std::cout << std::endl;

    // Clean up
    // delete[] mult_result;
    delete[] inv_result;
    //delete[] transpose_result;
    // delete[] add_result;

    return 0;
}
