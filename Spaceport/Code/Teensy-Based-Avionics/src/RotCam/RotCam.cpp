#include "RotCam.h"

#define SPEED 600
#define ACCELERATION 5000


RotCam::RotCam() : stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4)
{
    pinMode(HOME_SENS_PIN, INPUT);
    stepper.setMaxSpeed(SPEED);
    stepper.move(1);
    stepper.setAcceleration(ACCELERATION);
    stepper.setSpeed(SPEED);
    stepsPerRevolution = 1625;
}

void RotCam::moveToAngle(float angle, bool positive)
{
    if(angle == targetAngle)
        return;
    targetAngle = angle;
    int steps = angle * stepsPerRevolution / 360;
    stepper.moveTo(steps);
    printf("Moving to %f degrees\n", angle);
    printf("Steps: %d\n", steps);
}

void RotCam::home()
{
    while (detect())
        stepper.runToNewPosition(move(false));
    while (!detect())
        stepper.runToNewPosition(move(false));
    while (detect())
        stepper.runToNewPosition(move());
    stepper.runToNewPosition(move());
    stepper.runToNewPosition(move());
    stepper.setCurrentPosition(0);
    stepper.setSpeed(SPEED);
}

bool RotCam::detect() { return digitalRead(HOME_SENS_PIN) ? true : false; }

long RotCam::move(bool positive) { return stepper.currentPosition() + (positive ? 5 : -5); }

int RotCam::findStepsPerRevolution()
{
    int numSteps = 0;
    while (detect()){
        stepper.runToNewPosition(move());
    }
    while (!detect())
    {
        stepper.runToNewPosition(move());
        numSteps += 5;
    }
    while (detect())
    {
        stepper.runToNewPosition(move());
        numSteps += 5;
    }
    return numSteps;
}

void RotCam::run()
{
    stepper.run();
}