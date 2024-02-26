#ifndef MATRIXMULT_H
#define MATRIXMULT_H

#include <Arduino.h>
//#include "RFM69Helper.h"

double *multiplyMatrices(double *matrix1, double *matrix2, int matrix1_rows, int matrix1_cols, int matrix2_rows, int matrix2_cols);
double *inverseMatrix(double *matrix, int size);
double *transposeMatrix(double *matrix, int rows, int cols);
double *addMatrices(double *mat1, double *mat2, int rows, int cols);
double *subMatrices(double *mat1, double *mat2, int rows, int cols);
double *ident(int size);
double *multiplyByScalar(double *mat, int size, double scalar);

#endif