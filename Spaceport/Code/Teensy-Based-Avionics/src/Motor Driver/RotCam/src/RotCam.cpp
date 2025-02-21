#include "RotCam.h"

#define SPEED 600
#define ACCELERATION 5000

#define SERIAL_BAUD 115200
RotCam rotCam;
String inputString = "";
bool stringComplete = false;

// New Code
void setup() {
    Serial.begin(SERIAL_BAUD);
    inputString.reserve(200);
    rotCam.home();  // Home the camera on startup
}

void loop() {
    // Process any pending motor movements
    rotCam.run();

    // Process serial commands when a complete message is received
    if (stringComplete) {
        // Convert string to float and move to that angle
        float angle = inputString.toFloat();
        
        // Basic validation
        if (angle >= 0 && angle <= 360) {
            rotCam.moveToAngle(angle);
            Serial.print("Moving to angle: ");
            Serial.println(angle);
        } else {
            Serial.println("Error: Angle must be between 0 and 360 degrees");
        }

        // Clear the string for next input
        inputString = "";
        stringComplete = false;
    }
}

// SerialEvent occurs whenever new data comes in the hardware serial RX
void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        
        // Add character to input string if it's a number, decimal point, or negative sign
        if (isDigit(inChar) || inChar == '.' || inChar == '-') {
            inputString += inChar;
        }
        // If we get a newline, set a flag so the main loop can process the string
        else if (inChar == '\n') {
            stringComplete = true;
        }
    }
}


// Prewritten code

RotCam::RotCam() : stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4)
{
    pinMode(HOME_SENS_PIN, INPUT);
    stepper.setMaxSpeed(SPEED);
    stepper.move(1);
    stepper.setAcceleration(ACCELERATION);
    stepper.setSpeed(SPEED);
    stepsPerRevolution = findStepsPerRevolution();
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

int RotCam::getStepsPerRevolution()
{
    return stepsPerRevolution;
}