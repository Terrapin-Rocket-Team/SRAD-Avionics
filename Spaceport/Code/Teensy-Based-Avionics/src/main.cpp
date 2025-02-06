#include <SPI.h>
#include <Arduino.h>
#include "BMI088.h"

#include "AvionicsState.h"
#include "Pi.h"
#include "AvionicsKF.h"
#include "RadioMessage.h"
#include "RotCam/RotCam.h"

#define BMP_ADDR_PIN 36
#define RPI_CMD 34
#define RPI_RESP 35

using namespace mmfs;

Logger logger(15, 5);

MAX_M10S gps;
mmfs::DPS310 baro1;
mmfs::MS5611 baro2;
mmfs::BMI088andLIS3MDL bno;

APRSConfig aprsConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
APRSTelem aprs(aprsConfig);
Message msg;

Sensor *sensors[4] = {&gps, &bno, &baro1, &baro2};
AvionicsKF kfilter;

AvionicsState *computer; // = useKalmanFilter = true
uint32_t radioTimer = millis();
Pi rpi(RPI_CMD, RPI_RESP);
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
    Serial.print("\n");
    free(heapTop);
}
// Free memory debug function

const int BUZZER_PIN = LED_BUILTIN;
const int BUILTIN_LED_PIN = LED_BUILTIN;
int allowedPins[] = {BUILTIN_LED_PIN, BUZZER_PIN, 32};
BlinkBuzz bb(allowedPins, 3, true);
RotCam cam;

const int UPDATE_RATE = 10;
const int UPDATE_INTERVAL = 1000.0 / UPDATE_RATE;

double startTime = 0;

void checkRotCam(int stage, int vertVel);
void setup()
{
    Serial.begin(9600);
    Serial1.begin(460800);
    delay(3000);

    printf("beginning setup\n");    
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
    //bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    cam.home();
    char logData[100];
    snprintf(logData, 100, "Steps per revolution: %d", cam.getStepsPerRevolution());
    logger.recordLogData(INFO_, logData);
    rpi.startRec();
}
double radio_last;
void loop()
{
    double time = millis();

    bb.update();
    cam.run();
    rpi.check();

    // Update the state of the rocket
    if (time - last < 100)
        return;

    last = time;
    computer->updateState();
    checkRotCam(computer->getStage(), computer->getVelocity().z());
    logger.recordFlightData();
    if (gps.getHasFirstFix())
    {
        bb.clearQueue(32);
        bb.on(32);
    }

    Vector<3> orient = bno.getOrientation().toEuler() * 180 / PI;

    // printf("%.2f | %.2f | %.2f\n", orient.x(), orient.y(), orient.z());

    if (time - radio_last < 1000)
        return;

    radio_last = time;
    msg.clear();
    aprs.alt = baro1.getAGLAltFt();
    aprs.hdg = gps.getHeading();
    aprs.lat = gps.getPos().x();
    aprs.lng = gps.getPos().y();
    aprs.spd = computer->getVelocity().z();
    aprs.orient[0] = bno.getAngularVelocity().x();
    aprs.orient[1] = bno.getAngularVelocity().y();
    aprs.orient[2] = bno.getAngularVelocity().z();
    aprs.stateFlags = computer->getStage();
    msg.encode(&aprs);
    Message m(&aprs);
    APRSTelem aprs2(aprsConfig);
    m.decode(&aprs2);
    Serial1.write(msg.buf, msg.size);
    Serial1.write('\n');
    // printf("%0.7f | %0.7f | %d\n", aprs2.lat, aprs.lng, gps.getFixQual());

    // Serial.println(gps.getFixQual());

    // time, alt1, alt2, vel, accel, gyro, mag, lat, lon
    // printf("%.3f | %.2f, %.2f, %.2f | %.2f, %.2f, %.2f | %.2f, %.2f, %.2f \n",
    //        time / 1000.0,
    //        computer->getPosition().x(),
    //          computer->getPosition().y(),
    //             computer->getPosition().z(),
    //          computer->getVelocity().x(),
    //             computer->getVelocity().y(),
    //                computer->getVelocity().z(),
    //             computer->getAcceleration().x(),
    //                 computer->getAcceleration().y(),
    //                     computer->getAcceleration().z());
}

int lastStage = 0;
void checkRotCam(int stage, int vertVel)
{
    if (lastStage == stage)
        return;

    if(stage == 2 && computer->getTimeSinceLastStage() > 3 && vertVel <= 50) // about to hit apogee, look at parachute
    {
        cam.moveToAngle(0);
    }
    else if (stage == 2) // start coasting, look to horizon
    {
        cam.moveToAngle(90);
    }
    else if (stage == 3 && computer->getTimeSinceLastStage() > 3) // start drogue, look at ground (upside down)
    {
        cam.moveToAngle(180);
    }
    else if (stage == 4 && computer->getTimeSinceLastStage() > 3) // 5 sec after main, look at ground
    {
        cam.moveToAngle(180);
    }
    else if (stage == 4) // start main, look at chute
    {
        cam.moveToAngle(360);
        
    }
    else if(stage == 6)
    {
        rpi.stopRec();
    }
}
