#include <iostream>
#include <cmath>

#ifndef MATRIX_H
#define MATRIX_H


class Matrix {
  
public:
  Matrix();
  Matrix(int rows, int cols, double* array);
  ~Matrix();
  Matrix(const Matrix& other);
  Matrix& operator=(const Matrix& other);
  int getRows();
  int getCols();
  double* getArr();
  Matrix operator* (Matrix other);
  Matrix multiply (Matrix other);
  Matrix operator* (double scalar);
  Matrix multiply (double scalar);
  Matrix operator+ (Matrix other);
  Matrix add (Matrix other);
  Matrix operator- (Matrix other);
  Matrix subtract (Matrix other);
  Matrix T ();
  Matrix transpose ();
  Matrix inv ();
  Matrix inverse ();
  static Matrix ident (int n);
  void disp();
  
private:
  int rows;
  int cols;
  double* array;
  void luDecompositionWithPartialPivoting(double* A, int* pivot, int n);
  void solveLU(double* A, int* pivot, double* b, double* x, int n);
};

#endif
