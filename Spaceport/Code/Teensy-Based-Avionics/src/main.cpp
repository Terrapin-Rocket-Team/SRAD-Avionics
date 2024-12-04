#include <Arduino.h>
#include "BMI088.h"

#include "AvionicsState.h"
#include "Pi.h"
#include "AvionicsKF.h"

#define BMP_ADDR_PIN 36
#define RPI_PWR 0
#define RPI_VIDEO 1

using namespace mmfs;

Logger logger;

MAX_M10S gps;
mmfs::DPS310 baro1;
mmfs::MS5611 baro2;
mmfs::BMI088andLIS3MDL bno;

Sensor *sensors[4] = {&gps, &bno, &baro1, &baro2};
AvionicsKF kfilter;

AvionicsState *computer; // = useKalmanFilter = true
uint32_t radioTimer = millis();
Pi rpi(RPI_PWR, RPI_VIDEO);
PSRAM *psram;
ErrorHandler errorHandler;

static double last = 0; // for better timing than "delay(100)"

// Free memory debug function
extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

void FreeMem()
{
    void *heapTop = malloc(500);
    Serial.print((long)heapTop);
    Serial.print(" ");
    free(heapTop);
}
// Free memory debug function

const int BUZZER_PIN = 33;
const int BUILTIN_LED_PIN = LED_BUILTIN;
int allowedPins[] = {BUILTIN_LED_PIN, BUZZER_PIN, 32};
BlinkBuzz bb(allowedPins, 3, true);

const int UPDATE_RATE = 10;
const int UPDATE_INTERVAL = 1000.0 / UPDATE_RATE;

void setup()
{
    Serial.begin(9600);
    delay(3000);
    Wire.begin();
    SENSOR_BIAS_CORRECTION_DATA_LENGTH = 2;
    SENSOR_BIAS_CORRECTION_DATA_IGNORE = 1;
    computer = new AvionicsState(sensors, 4, nullptr);

    psram = new PSRAM();

    logger.init(computer);

    logger.recordLogData(INFO_, "Initializing Avionics System.", TO_USB);

    if (CrashReport)
    {
        Serial.println(CrashReport);
    }
    // The SD card MUST be initialized first to allow proper data logging.
    if (logger.isSdCardReady())
    {

        logger.recordLogData(INFO_, "SD Card Initialized");
        bb.onoff(BUZZER_PIN, 1000);
    }
    else
    {
        logger.recordLogData(ERROR_, "SD Card Failed to Initialize");

        bb.onoff(BUZZER_PIN, 200, 3);
    }

    // The PSRAM must be initialized before the sensors to allow for proper data logging.

    if (logger.isPsramReady())
        logger.recordLogData(INFO_, "PSRAM Initialized");
    else
        logger.recordLogData(ERROR_, "PSRAM Failed to Initialize");

    if (computer->init(true))
    {
        logger.recordLogData(INFO_, "All Sensors Initialized");
        bb.onoff(BUZZER_PIN, 1000);
    }
    else
    {
        logger.recordLogData(ERROR_, "Some Sensors Failed to Initialize. Disabling those sensors.");
        bb.onoff(BUZZER_PIN, 200, 3);
    }
    logger.writeCsvHeader();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); //blink a status LED (until GPS fix)
}

void loop()
{
    double time = millis();
    bb.update();
    // Update the state of the rocket
    if (time - last < 100)
        return;

    last = time;
    computer->updateState();
    // time, alt1, alt2, vel, accel, gyro, mag, lat, lon
    // printf("%.2f | %.2f | %.2f, %.2f # %.2f, %.2f | %.2f, %.2f, %.2f | %.2f, %.2f, %.2f = %.2f, %.2f, %.2f | %.2f, %.2f, %.2f | %.7f, %.7f\n",
    //        time / 1000.0,
    //        computer->getPosition().z(),
    //        baro1.getASLAltM(),
    //        baro1.getPressure(),
    //        baro2.getASLAltM(),
    //        baro2.getPressure(),
    //        computer->getVelocity().x(),
    //        computer->getVelocity().y(),
    //        computer->getVelocity().z(),
    //        bno.getAcceleration().x(),
    //        bno.getAcceleration().y(),
    //        bno.getAcceleration().z(),
    //        bno.getAngularVelocity().x(),
    //        bno.getAngularVelocity().y(),
    //        bno.getAngularVelocity().z(),
    //        bno.getMagField().x(),
    //        bno.getMagField().y(),
    //        bno.getMagField().z(),
    //        gps.getPos().x(),
    //        gps.getPos().y());
    printf("%.2f | %.2f = %.2f | %.2f\n", baro1.getASLAltFt(), baro2.getASLAltFt(), baro1.getAGLAltFt(), baro2.getAGLAltFt());
    logger.recordFlightData();
    if(gps.getHasFirstFix()){
        bb.clearQueue(32);
        bb.on(32);
    }
}
