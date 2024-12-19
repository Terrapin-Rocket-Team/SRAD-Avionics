#include "RotCam.h"

#define SPEED 1000


void setupp(){
    pinMode(HOME_SENS_PIN, INPUT);

      stepper->setMaxSpeed(1000.0);
  stepper->setAcceleration(1000.0);
}

void home() {
    if(digitalRead(HOME_SENS_PIN) == HIGH) {
        stepper->setSpeed(-SPEED);
    } else {
        stepper->setSpeed(SPEED);
    }
}