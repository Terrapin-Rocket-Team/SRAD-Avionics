#ifndef ROTCAM_H
#define ROTCAM_H

#include <Arduino.h>
#include <AccelStepper.h>
#define HOME_SENS_PIN 23
#define STEPPER_PIN_1 2
#define STEPPER_PIN_2 3
#define STEPPER_PIN_3 4
#define STEPPER_PIN_4 5

extern AccelStepper *stepper;

void setupp();
void home();

#endif