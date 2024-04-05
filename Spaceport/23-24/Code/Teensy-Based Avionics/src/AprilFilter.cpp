Matrix get_F(int dt){
  double* result = new double[36]{1, 0, 0, dt, 0, 0,
      0, 1, 0, 0, dt, 0,
      0, 0, 1, 0, 0, dt,
      0, 0, 0, 1, 0, 0,
      0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 1};
  return Matrix(6, 6, result);
}

Matrix get_G(int dt){
  double* result = new double[18]{0.5 * dt * dt, 0, 0,
      0, 0.5 * dt * dt, 0,
      0, 0, 0.5 * dt * dt,
      dt, 0, 0,
      0, dt, 0,
      0, 0, dt};
  return Matrix(6, 3, result);
}

Matrix get_H(int has_gps, int has_barometer){
  int sum = (has_barometer + has_gps) == 0 ? 1 : (has_barometer + has_gps);
  double* result = new double[18]{1.0 * has_gps, 0, 0, 0, 0, 0,
      0, 1.0 * has_gps, 0, 0, 0, 0,
      0, 0, 1.0 / sum * has_gps, 0, 0, 0,
      0, 0, 1.0 / sum * has_barometer, 0, 0, 0};
  return Matrix(3, 6, result);
}
