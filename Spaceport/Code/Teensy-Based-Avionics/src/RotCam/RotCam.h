#ifndef ROTCAM_H
#define ROTCAM_H

#include <Arduino.h>
#include <AccelStepper.h>
#define HOME_SENS_PIN 22
#define STEPPER_PIN_1 38
#define STEPPER_PIN_2 39
#define STEPPER_PIN_3 40
#define STEPPER_PIN_4 41


class RotCam
{
private:
    AccelStepper stepper;
    int stepsPerRevolution;
    bool detect();
    long move(bool positive = true);
    float targetAngle;

public:
    RotCam();
    void moveToAngle(float angle, bool positive = true);
    void home();
    int findStepsPerRevolution();
    void run();


};

#endif