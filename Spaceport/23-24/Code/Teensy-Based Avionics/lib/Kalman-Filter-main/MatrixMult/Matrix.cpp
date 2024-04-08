#include "Matrix.h"

//Default constructor
Matrix::Matrix(){
  this->rows = 0;
  this->cols = 0;
  this->array = nullptr;
}

//Constructor
// int rows -> number of rows of matrix
// int cols -> number of columns of matrix
// double[] array -> array of elements of matrix in column-major order
//IMPORTANT: DO NOT modify array after use in constructor!
Matrix::Matrix(int rows, int cols, double* array) {
  this->rows = rows;
  this->cols = cols;
  this->array = array;
}

//Destructor
Matrix::~Matrix(){
  delete[] this->array;
}

Matrix::Matrix(const Matrix& other) {
    rows = other.rows;
    cols = other.cols;
    array = new double[rows * cols];
    std::copy(other.array, other.array + rows * cols, array);
}

Matrix& Matrix::operator=(const Matrix& other) {
    if (this != &other) { // self-assignment check
        delete[] array; // free existing resource
        rows = other.rows;
        cols = other.cols;
        array = new double[rows * cols];
        std::copy(other.array, other.array + rows * cols, array);
    }
    return *this;
}

//Gets #rows of the matrix
int Matrix::getRows(){
  return this->rows;
}

//Gets #columns of the matrix
int Matrix::getCols(){
  return this->cols;
}

double* Matrix::getArr(){
  return this->array;
}

//Overloads * operator to multiply two matrices
Matrix Matrix::operator*(Matrix other){
  return this->multiply(other);
}

//Overloads * operator to multiply a matrix and a scalar
// Note: The scalar must be on the right hand side of the *
Matrix Matrix::operator*(double scalar){
  return this->multiply(scalar);
}

//Multiply two matrices
// Matrix other -> matrix to multiply with
// return -> the product
Matrix Matrix::multiply(Matrix other){
  if (this->cols != other.rows){
    std::cerr << "Multiplication error: Dimensions do not match!" << std::endl;
  }

  double* result = new double[this->rows * other.cols];

  for (int i = 0; i < this->rows; ++i){
    for (int j = 0; j < other.cols; ++j){
      result[i * other.cols + j] = 0;
      for (int k = 0; k < this->cols; ++k){
	result[i * other.cols + j] += this->array[i * this->cols + k] * other.array[k * other.cols + j];
      }
    }
  }

  return Matrix(this->rows, other.cols, result); //returns product matrix
}

//Multiply a matrix and a scalar
// double scalar -> scalar to multiply by
// return -> the product
Matrix Matrix::multiply(double scalar){
  double* result = new double[this->rows * this->cols];

  for (int i = 0; i < this->rows * this->cols; ++i){
    result[i] = this->array[i] * scalar;
  }

  return Matrix(this->rows, this->cols, result); //returns product matrix
}

//Overloads the + operator to add two matrices
Matrix Matrix::operator+(Matrix other){
  return this->add(other);
}

//Add two matrices
// Matrix other -> matrix to add
// return -> sum of matrices
Matrix Matrix::add(Matrix other){
  if(this->rows != other.rows || this->cols != other.cols){
    std::cerr << "Addition error: Dimensions do not match!" << std::endl;
  }
  
  double* result = new double[this->rows * this->cols];

  for (int i = 0; i < this->rows * this->cols; ++i){
    result[i] = this->array[i] + other.array[i];
  }

  return Matrix(this->rows, this->cols, result); //returns sum matrix
}

//Overloads the - operator to subtract two matrices
Matrix Matrix::operator-(Matrix other){
  return this->subtract(other);
}

//Subtract two matrices
// Matrix other -> matrix to subtract
// return -> difference of matrices
Matrix Matrix::subtract(Matrix other){
  if(this->rows != other.rows || this->cols != other.cols){
    std::cerr << "Subtraction error: Dimensions do not match!" << std::endl;
  }

  double* result = new double[this->rows * this->cols];

  for (int i = 0; i < this->rows * this->cols; ++i){
    result[i] = this->array[i] - other.array[i];
  }

  return Matrix(this->rows, this->cols, result); //returns difference matrix
}

//Alias for transpose()
Matrix Matrix::T(){
  return this->transpose();
}

//Transpose a matrix
// return -> transposed matrix
Matrix Matrix::transpose(){
  double* result = new double[this->rows * this->cols];

  for (int i = 0; i < this->rows; ++i){
    for (int j = 0; j < this->cols; ++j){
      result[j * this->rows + i] = this->array[i * this->cols + j];
    }
  }

  return Matrix(this->cols, this->rows, result); //returns transposed matrix
}

// From Avi's code to invert a matrix; copied verbatim
void Matrix::luDecompositionWithPartialPivoting(double* A, int* pivot, int n) {
  for (int i = 0; i < n; ++i) {
    pivot[i] = i;
  }

  for (int i = 0; i < n; ++i) {
    // Partial pivoting
    double max = std::abs(A[i*n + i]);
    int maxRow = i;
    for (int k = i + 1; k < n; ++k) {
      if (std::abs(A[k*n + i]) > max) {
	max = std::abs(A[k*n + i]);
	maxRow = k;
      }
    }

    if (max == 0.0) {
      std::cerr << "Inversion error: Matrix is singular!" << std::endl;
      return;
    }

    // Swap rows in A matrix
    for (int k = 0; k < n; ++k) {
      std::swap(A[i*n + k], A[maxRow*n + k]);
    }
    // Swap pivot indices
    std::swap(pivot[i], pivot[maxRow]);

    // LU Decomposition
    for (int j = i + 1; j < n; ++j) {
      A[j*n + i] /= A[i*n + i];
      for (int k = i + 1; k < n; ++k) {
	A[j*n + k] -= A[j*n + i] * A[i*n + k];
      }
    }
  }
}

// From Avi's code to invert a matrix; copied verbatim
void Matrix::solveLU(double* A, int* pivot, double* b, double* x, int n) {
  // Forward substitution for Ly = Pb
  for (int i = 0; i < n; ++i) {
    x[i] = b[pivot[i]];
    for (int j = 0; j < i; ++j) {
      x[i] -= A[i*n + j] * x[j];
    }
  }

  // Backward substitution for Ux = y
  for (int i = n - 1; i >= 0; --i) {
    for (int j = i + 1; j < n; ++j) {
      x[i] -= A[i*n + j] * x[j];
    }
    x[i] /= A[i*n + i];
  }
}

//Alias for inverse()
Matrix Matrix::inv(){
  return this->inverse();
}

//Invert a matrix
// return -> inverted matrix
Matrix Matrix::inverse(){
  if(this->rows != this->cols){
    std::cerr << "Inversion error: Dimensions do not match!" << std::endl;
  }

  int n = this->rows; //at this point, n = rows = cols
  double* A = new double[n * n]; //makes a copy of the array to avoid modification
  for (int i = 0; i < n * n; ++i){
    A[i] = this->array[i];
  }

  //below code closely follows Avi's code
  double* inverse = new double[n * n]; 
  int* pivot = new int[n];
  double* b = new double[n];
  double* temp = new double[n];

  this->luDecompositionWithPartialPivoting(A, pivot, n);

  for (int i = 0; i < n; ++i){
    std::fill(b, b + n, 0.0);
    b[i] = 1.0;

    this->solveLU(A, pivot, b, temp, n);

    for (int j = 0; j < n; ++j){
      inverse[j * n + i] = temp[j];
    }
  }

  delete[] A;
  delete[] pivot;
  delete[] b;
  delete[] temp;

  return Matrix(n, n, inverse); //returns inverted matrix
}

//Get identity matrix of size [n n]
Matrix Matrix::ident(int n){
  double* result = new double[n * n];

  for (int i = 0; i < n; ++i){
    for (int j = 0; j < n; ++j){
      if (i == j){
	result[j * n + i] = 1;
      } else{
	result[j * n + i] = 0;
      }
    }
  }

  return Matrix(n, n, result); //returns identity matrix
}

//Display matrix for debugging purposes
void Matrix::disp(){
  //prints each row of the matrix on a separate line
  for (int i = 0; i < this->rows; ++i){
    //separates each column of the matrix by a space
    for (int j = 0; j < this->cols; ++j){
      std::cout << this->array[i * this->cols + j] << " ";
    }
    std::cout << std::endl;
  }
}
