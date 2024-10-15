/*
Create your main file that will use your custom sensor, filter and state classes.
*/

#include <Arduino.h>
#include <BlinkBuzz/BlinkBuzz.h>
#include <Sensors/Baro/BMP390.h>
#include "FIRFilter.h"
#include "MyState.h"
#include "BMP280.h"

using namespace mmfs;

const int BUILTIN_LED_PIN = LED_BUILTIN;
int allowedPins[] = {BUILTIN_LED_PIN};
BlinkBuzz bb(allowedPins, 1, true);

//-------------------Implement Code Here-------------------//

// instantiate your custom sensor, filter and state classes here

// create Sensor pointer array for all sensors to pass to your state class

Logger logger;

void setup()
{

    logger.init();

    // initialize things here
}

void loop()
{
    bb.update();

    // update your state here
}