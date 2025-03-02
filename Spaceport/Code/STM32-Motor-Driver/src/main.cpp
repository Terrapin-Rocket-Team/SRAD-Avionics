#include <Arduino.h>
#include <AccelStepper.h>

#define STEPPER_PIN_1 PA4
#define STEPPER_PIN_2 PA5
#define STEPPER_PIN_3 PA6
#define STEPPER_PIN_4 PA7

#define HOME_SENS_PIN PA3

/******************************************************
 * Stepper Configuration
 ******************************************************/
// Create AccelStepper instance (FULL4WIRE sequence).
// Order: pin1, pin3, pin2, pin4 so that coil1=pin1&pin2, coil2=pin3&pin4
AccelStepper stepper(AccelStepper::FULL4WIRE,
                     STEPPER_PIN_1, STEPPER_PIN_3,
                     STEPPER_PIN_2, STEPPER_PIN_4);

// Tweak these as needed
static const float SPEED = 600.0;         // steps/s
static const float ACCELERATION = 5000.0; // steps/s^2
static const long STEP_MOVE_INC = 25;      // how many steps to move in each incremental move

/******************************************************
 * Globals
 ******************************************************/
long stepsPerRevolution = 2050; // Measured via findStepsPerRevolution()
float targetAngle = 0.0;

int seconds_since_last_move = 0;

/******************************************************
 * detect()
 * - Returns true if Hall sensor is triggered (active low)
 ******************************************************/
bool detect()
{
    // Active low means digitalRead(...) == LOW => triggered
    return (digitalRead(HOME_SENS_PIN) == LOW);
}



/******************************************************
 * moveToAngle(angle)
 * - Convert angle (0..360) to steps, then move
 ******************************************************/
void moveToAngle(float angle)
{
    if (stepsPerRevolution <= 0)
    {
        Serial1.println("ERROR: stepsPerRevolution not set. Please home or manually set it!");
        return;
    }
    if (angle == targetAngle)
    {
        // no change
        return;
    }

    targetAngle = angle;
    long steps = (long)((angle / 360.0) * (float)stepsPerRevolution);
    stepper.moveTo(steps);

    Serial1.print("Moving to ");
    Serial1.print(angle);
    Serial1.println(" degrees...");
    Serial1.print("Target steps: ");
    Serial1.println(steps);
}

/******************************************************
 * home()
 * - Move stepper until Hall sensor is triggered
 * - Then sets currentPosition to 0
 * - This can be refined to approach from one side, etc.
 ******************************************************/
void home()
{
    Serial1.println("Starting homing routine...");

    // 1) If sensor is already triggered, back off until untriggered
    while (detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() - STEP_MOVE_INC);
    }

    // 2) Move forward until triggered
    while (!detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() + STEP_MOVE_INC);
    }

    // Optionally refine approach or back off slightly:
    // Move backward a bit if needed, etc.
    // For example, back off until untriggered again:
    while (detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() - STEP_MOVE_INC);
    }
    stepper.runToNewPosition(stepper.currentPosition() + STEP_MOVE_INC * 6);
    // Now set zero
    stepper.setCurrentPosition(0);

    Serial1.println("Homing complete. Current position = 0");
}

/******************************************************
 * parseSerialCommands()
 * - Expects either a numeric angle (float) or "HOME"
 *   e.g. you can send:  45.0   or   180   or   HOME
 ******************************************************/
float angle = 0;
void parseSerialCommands()
{
    static String inputString = "";

    // // If there is data available, read it
    // while (Serial1.available())
    // {
    //     char inChar = (char)Serial1.read();

    //     // If we get newline, we parse the input
    //     if (inChar == '\n' || inChar == '\r')
    //     {
    //         if (inputString.length() > 0)
    //         {
    //             // Check if command is "HOME" (case-insensitive)
    //             if (inputString.equalsIgnoreCase("HOME"))
    //             {
    //                 Serial1.println("Homing stepper...");
    //                 home();
    //             }
    //             else
    //             {
    //                 // Try to parse a float angle
    //                 float angle = inputString.toFloat();
    //                 moveToAngle(angle);
    //             }
    //         }
    //         inputString = ""; // clear buffer
    //     }
    //     else
    //     {
    //         // Accumulate characters into the string
    //         inputString += inChar;
    //         Serial1.println(inputString);
    //     }
    // }

    // every 5 seconds, move to a random angle
    if ((millis() - seconds_since_last_move > 5000) && (millis() < 60000))
    {
        seconds_since_last_move = millis();
        angle += 90;
        moveToAngle(angle);
    }
    else if(millis() >= 60000){
        moveToAngle(((int)angle / 360) * 360);
    }
}

/******************************************************
 * findStepsPerRevolution()
 * - Example routine to measure the total steps in 360 deg
 *   by detecting the Hall sensor transitions in one full rotation
 ******************************************************/
long findStepsPerRevolution()
{
    long numSteps = 0;

    Serial1.println("Finding steps per revolution. Rotating until sensor triggered twice...");

    // 1) If sensor is triggered, move away so we start from untriggered
    while (detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() + STEP_MOVE_INC);
    }

    // 2) Move until triggered
    while (!detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() + STEP_MOVE_INC);
        numSteps += STEP_MOVE_INC;
    }

    // 3) Move until untriggered
    while (detect())
    {
        stepper.runToNewPosition(stepper.currentPosition() + STEP_MOVE_INC);
        numSteps += STEP_MOVE_INC;
    }

    // That “first transition” presumably marks a single magnet crossing
    // If your sensor is guaranteed to trigger only once per revolution,
    // then the above “numSteps” might be your steps/rev.
    // If your sensor triggers multiple times per rev, you might need to
    // repeat until a second trigger to get the full 360.

    Serial1.print("Approx steps measured so far: ");
    Serial1.println(numSteps);

    return numSteps;
}


/******************************************************
 * Setup
 ******************************************************/
void setup()
{
    // Initialize serial (pins PA9=TX, PA10=
    Serial1.begin(115200);
    while (!Serial)
    { /* Wait for serial if needed */
    }

    // Set up Hall sensor pin
    pinMode(HOME_SENS_PIN, INPUT_PULLUP);
    // If your sensor is open-drain or you need an external pullup,
    // you might change this to INPUT or use external resistor.

    // Configure stepper
    stepper.setMaxSpeed(SPEED);
    stepper.setAcceleration(ACCELERATION);

    Serial1.println("RotCam STM32F103C8 Example Starting...");

    home();
    delay(5000);

    // Optional: Find how many steps in one revolution
    // (Uncomment if you want to measure steps/rev automatically)
    // stepsPerRevolution = findStepsPerRevolution();
    // Serial1.print("Detected steps per revolution: ");
    // Serial1.println(stepsPerRevolution);

    // If you already know your steps per revolution, just set it:
    // stepsPerRevolution = 2048; // example for 28BYJ-48 or 200 for typical NEMA17, etc.
    // stepper.setCurrentPosition(0);
}

/******************************************************
 * Loop
 ******************************************************/
void loop()
{
    // 1) Check for incoming serial commands
    parseSerialCommands();

    // 2) Continuously run the stepper (must be called frequently)
    stepper.run();
}