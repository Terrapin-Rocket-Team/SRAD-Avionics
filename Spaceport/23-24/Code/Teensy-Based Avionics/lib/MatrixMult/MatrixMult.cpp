#include "MatrixMult.h"

double *multiplyMatrices(double *matrix1, double *matrix2, int matrix1_rows, int matrix1_cols, int matrix2_rows, int matrix2_cols)
{
    if (matrix1_cols != matrix2_rows)
    {
        Serial.println("Error: Columns in matrix1 must match rows in matrix2.");
        return nullptr;
    }

    double *result = new double[matrix1_rows * matrix2_cols];

    for (int i = 0; i < matrix1_rows; ++i)
    {
        for (int j = 0; j < matrix2_cols; ++j)
        {
            result[i * matrix2_cols + j] = 0;
            for (int k = 0; k < matrix1_cols; ++k)
            {
                result[i * matrix2_cols + j] += matrix1[i * matrix1_cols + k] * matrix2[k * matrix2_cols + j];
            }
        }
    }

    return result;
}

// Uses Gaussian Elimination with Partial Pivoting to find the inverse of a matrix
double *inverseMatrix(double *matrix, int size)
{
    double *result = new double[size * size];

    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            result[i * size + j] = (i == j) ? 1.0 : 0.0;
        }
    }

    for (int i = 0; i < size; ++i)
    {
        int pivotRow = i;
        for (int j = i + 1; j < size; ++j)
        {
            if (abs(matrix[j * size + i]) > abs(matrix[pivotRow * size + i]))
            {
                pivotRow = j;
            }
        }

        if (matrix[pivotRow * size + i] == 0.0)
        {
            Serial.println("Error: Matrix is singular, cannot calculate inverse.");
            return nullptr;
        }

        if (pivotRow != i)
        {
            for (int k = 0; k < size; ++k)
            {
                // Manually swap matrix elements
                double tempMatrix = matrix[i * size + k];
                matrix[i * size + k] = matrix[pivotRow * size + k];
                matrix[pivotRow * size + k] = tempMatrix;

                // Manually swap result elements
                double tempResult = result[i * size + k];
                result[i * size + k] = result[pivotRow * size + k];
                result[pivotRow * size + k] = tempResult;
            }
        }

        double pivot = matrix[i * size + i];
        for (int j = 0; j < size; ++j)
        {
            matrix[i * size + j] /= pivot;
            result[i * size + j] /= pivot;
        }

        for (int j = 0; j < size; ++j)
        {
            if (j != i)
            {
                double factor = matrix[j * size + i];
                for (int k = 0; k < size; ++k)
                {
                    matrix[j * size + k] -= factor * matrix[i * size + k];
                    result[j * size + k] -= factor * result[i * size + k];
                }
            }
        }
    }

    return result;
}

double *transposeMatrix(double *matrix, int rows, int cols)
{
    double *result = new double[rows * cols];

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            result[j * rows + i] = matrix[i * cols + j];
        }
    }

    return result;
}

double *addMatrices(double *mat1, double *mat2, int rows, int cols)
{
    double *result = new double[rows * cols];

    for (int i = 0; i < rows * cols; ++i)
    {
        result[i] = mat1[i] + mat2[i];
    }

    return result;
}

double *subMatrices(double *mat1, double *mat2, int rows, int cols)
{
    double *result = new double[rows * cols];

    for (int i = 0; i < rows * cols; ++i)
    {
        result[i] = mat1[i] - mat2[i];
    }

    return result;
}

double *ident(int size)
{
    double *result = new double[size * size];

    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            if (i == j)
            {
                result[i * size + j] = 1;
            }
            else
            {
                result[i * size + j] = 0;
            }
        }
    }

    return result;
}

double *multiplyByScalar(double *mat, int size, double scalar)
{
    double *result = new double[size];

    for (int i = 0; i < size; ++i)
    {
        result[i] = mat[i] * scalar;
    }

    return result;
}