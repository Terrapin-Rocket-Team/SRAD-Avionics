#include <Arduino.h>
#include "BMI088.h"

#include "AvionicsState.h"
#include "Pi.h"
#include "AvionicsKF.h"
#include "RadioMessage.h"
#include "Si4463.h"

#define BMP_ADDR_PIN 36
#define RPI_PWR 0
#define RPI_VIDEO 1

using namespace mmfs;

Logger logger(15, 5);

MAX_M10S gps;
mmfs::DPS310 baro1;
mmfs::MS5611 baro2;
mmfs::BMI088andLIS3MDL bno;

APRSConfig aprsConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
    uint8_t encoding[] = {7, 4, 4};
    APRSTelem aprs(aprsConfig);

Message msg;

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_500b,   // data rate
    433e6,     // frequency (Hz)
    127,       // tx power (127 = ~20dBm)
    48,        // preamble length
    16,        // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);

Sensor *sensors[4] = {&gps, &bno, &baro1, &baro2};
AvionicsKF kfilter;

AvionicsState *computer; // = useKalmanFilter = true
uint32_t radioTimer = millis();
Pi rpi(RPI_PWR, RPI_VIDEO);
PSRAM *psram;
ErrorHandler errorHandler;

static double last = 0; // for better timing than "delay(100)"
bool gpsHasFix = false;

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

const int BUZZER_PIN = 0;
const int BUILTIN_LED_PIN = LED_BUILTIN;
int allowedPins[] = {BUILTIN_LED_PIN, BUZZER_PIN};
BlinkBuzz bb(allowedPins, 2, true);

const int UPDATE_RATE = 10;
const int UPDATE_INTERVAL = 1000.0 / UPDATE_RATE;

void setup()
{
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    Serial.begin(9600);
    delay(3000);
    Wire.begin();
    SENSOR_BIAS_CORRECTION_DATA_LENGTH = 2;
    SENSOR_BIAS_CORRECTION_DATA_IGNORE = 1;
    computer = new AvionicsState(sensors, 4, &kfilter);

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
    // bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)

    if (radio.begin())
    {
        bb.onoff(BUZZER_PIN, 1000);
        logger.recordLogData(ERROR_, "Radio initialized.");
    }
    else
    {
        bb.onoff(BUZZER_PIN, 200, 3);
        logger.recordLogData(INFO_, "Radio failed to initialize.");
    }

    logger.recordLogData(INFO_, "Initialization Complete");
}
double radio_last;
void loop()
{
    double time = millis();
    bb.update();
    radio.update();
    // Update the state of the rocket
    if (time - last < 100)
        return;

    last = time;
    computer->updateState();

    logger.recordFlightData();

    // if (gps.getFixQual() > 0 && !gpsHasFix)
    // {
    //     gpsHasFix = true;
    //     bb.clearQueue(32);
    //     bb.on(32);
    // }
    // else if (gpsHasFix)
    // {
    //     gpsHasFix = false;
    //     bb.clearQueue(32);
    //     bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    // }

    if (time - radio_last < 1000)
        return;

    radio_last = time;
    msg.clear();

    printf("%f\n", baro1.getAGLAltFt());
    aprs.alt = baro1.getAGLAltFt();
    printf("%f\n", gps.getHeading());
    aprs.hdg = gps.getHeading();
    printf("%f\n", gps.getPos().x());
    aprs.lat = gps.getPos().x();
    printf("%f\n", gps.getPos().y());
    aprs.lng = gps.getPos().y();
    printf("%f\n", computer->getVelocity().z());
    aprs.spd = computer->getVelocity().z();
    printf("%f\n", bno.getAngularVelocity().x());
    aprs.orient[0] = bno.getAngularVelocity().x();
    printf("%f\n", bno.getAngularVelocity().y());
    aprs.orient[1] = bno.getAngularVelocity().y();
    printf("%f\n", bno.getAngularVelocity().z());
    aprs.orient[2] = bno.getAngularVelocity().z();
    aprs.stateFlags.setEncoding(encoding, 3);

    uint8_t arr[] = {(uint8_t) (int) baro1.getTemp(), (uint8_t)computer->getStage(), (uint8_t)gps.getFixQual()};   
    aprs.stateFlags.pack(arr);
    // aprs.stateFlags = (uint8_t) computer->getStage();
    msg.encode(&aprs);
    radio.send(aprs);
    Serial.println("Sent APRS Message");
    Serial.flush();
    bb.aonoff(BUZZER_PIN, 50);
    //  Serial1.write(msg.buf, msg.size);
    //  Serial1.write('\n');
}
