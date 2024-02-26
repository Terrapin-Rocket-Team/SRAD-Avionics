//HOW TO GET IT WORKING
//first, create a KFState object that you will be using
//create the initial matrices and call init
//create a size 10 double array
// - this is where you will get the values from
// - the first index is time, the next 9 are x y z pos vel acc
//create a size 4 measurements array and a size 3 inputs array
// - the measurements array should contain gps x y z and barometer z position
// - the inputs array should contain imu x y z acceleration
// - if there is no gps x y z they can be any value
// call updateFilter the way I have
// - if there is no gps make has_gps 0!
// your values will be in the predictions array after calling

//#include <Arduino.h>
#include "AbhiKalmanFilter.h"

KFState *state = new KFState();


void setup()
{
  double *initial_state = new double[6]{0, 0, 0, 0, 0, 0};
  double *initial_input = new double[3]{0, 0, 0};
  double *initial_covariance = new double[36]{1, 0, 0, 1, 0, 0,
                                              0, 1, 0, 0, 1, 0,
                                              0, 0, 1, 0, 0, 1,
                                              1, 0, 0, 1, 0, 0,
                                              0, 1, 0, 0, 1, 0,
                                              0, 0, 1, 0, 0, 1};
  double *measurement_covariance = new double[16]{3, 0, 0, 0,
                                                  0, 3, 0, 0,
                                                  0, 0, 3, 0,
                                                  0, 0, 0, 3};
  double *process_noise_covariance = new double[36]{.1, 0, 0, 0, 0, 0,
                                                    0, .1, 0, 0, 0, 0,
                                                    0, 0, .1, 0, 0, 0,
                                                    0, 0, 0, .1, 0, 0,
                                                    0, 0, 0, 0, .1, 0,
                                                    0, 0, 0, 0, 0, .1};
  init(state, 6, 3, 4, initial_state, initial_input, initial_covariance, measurement_covariance, process_noise_covariance);
}

void loop()
{
  //time pos x y z vel x y z acc x y z
  double *predictions = new double[10]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  //gps x y z barometer z
  double *measurements = new double[4]{1, 1, 1, 1};
  //imu x y z
  double *inputs = new double[3]{1, 1, 1};
  double time = 1;
  updateFilter(state, time, 1, measurements, inputs, &predictions);
  delete [] predictions;
  delete [] measurements;
  delete [] inputs;
}

int main(int argc, char **argv)
{
  setup();
  while (true)
  {
    loop();
  }
  return 0;
}