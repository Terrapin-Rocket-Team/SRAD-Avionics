#include "Matrix.h"
#include "MatrixMult.cpp"

int main() {
  double* test = new double[4]{1.0, 2.0, 3.0, 4.0};
  Matrix a = Matrix(2, 2, test);
  Matrix b = a * 5;
  Matrix c = a * b;
  Matrix d = c - b;
  Matrix e = d + a;
  Matrix f = e.inv();
  Matrix g = f.T();
  Matrix h = Matrix::ident(2);
  
  double* a2 = new double[4]{1.0, 2.0, 3.0, 4.0};
  double* b2 = multiplyByScalar(a2, 4, 5);
  double* c2 = multiplyMatrices(a2, b2, 2, 2, 2, 2);
  double* d2 = subMatrices(c2, b2, 2, 2);
  double* e2 = addMatrices(d2, a2, 2, 2);
  double* f2 = inverseMatrix(e2, 2);
  double* g2 = transposeMatrix(f2, 2, 2);
  double* h2 = ident(2);
  
  std::cout << "a" << std::endl;
  a.disp();
  std::cout << "a2" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << a2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  
  std::cout << "b = a * 5" << std::endl;
  b.disp();
  std::cout << "b2 = a2 * 5" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << b2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  
  std::cout << "c = a * b" << std::endl;
  c.disp();
  std::cout << "c2 = a2 * b2" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << c2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  
  std::cout << "d = c - b" << std::endl;
  d.disp();
  std::cout << "d2 = c2 - b2" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << d2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "e = d + a" << std::endl;
  e.disp();
  std::cout << "e2 = d2 + a2" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << e2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "f = inv(e)" << std::endl;
  f.disp();
  std::cout << "f2 = inv(e2)" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << f2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "g = f'" << std::endl;
  g.disp();
  std::cout << "g2 = f2'" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << g2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "h = eye(2)" << std::endl;
  h.disp();
  std::cout << "h2 = eye(2)" << std::endl;
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 2; j++){
      std::cout << h2[i * 2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  
}
