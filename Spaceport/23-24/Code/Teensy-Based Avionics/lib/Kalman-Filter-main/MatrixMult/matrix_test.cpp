#include "Matrix.h"

int main(){
    Matrix A(3, 3, new double[9]{1, 2, 3, 4, 5, 6, 7, 2, 9});
    Matrix B(3, 3, new double[9]{1, 2, 3, 4, 5, 6, 7, 8, 9});
    Matrix initialState(6, 1, new double[6] {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    Matrix D(3, 2, new double[6]{1, 2, 3, 4, 5, 6});
    Matrix F(2, 3, new double[6]{1, 2, 3, 4, 5, 6});
    D.disp();
    F.disp();
    initialState.disp();
    D.T().disp();
}