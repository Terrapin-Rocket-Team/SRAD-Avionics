#include <AccelStepper.h>

// Pin Definitions
#define HOME_SENS_PIN 23  // Home sensor pin
#define STEPPER_PIN_1 2  // Stepper motor pin 1
#define STEPPER_PIN_2 3  // Stepper motor pin 2
#define STEPPER_PIN_3 4  // Stepper motor pin 3
#define STEPPER_PIN_4 5  // Stepper motor pin 4

// Constants
#define SPEED 500        // Motor speed
#define ACCELERATION 5000
#define STEP_DISTANCE 5000

// Define stepper motor (full 4-wire interface)
AccelStepper stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4); // REVERSE PINS 2 AND 3!!!!!!!!!

void setup() {
    pinMode(HOME_SENS_PIN, INPUT); // Home sensor pin setup

    // Stepper motor setup
    stepper.setMaxSpeed(SPEED);
    stepper.setAcceleration(ACCELERATION);
    stepper.move(1);
    stepper.setSpeed(SPEED);

}
void loop() {
    // switch speed based on sensor high or low

    if (digitalRead(HOME_SENS_PIN) == HIGH) {
        stepper.setSpeed(SPEED);
    } else {
        stepper.setSpeed(-SPEED);
    }
    

    // Run stepper to its target position
    stepper.runSpeed();
}
