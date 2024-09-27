#include <Arduino.h>
#include <Sensors/BMI088.h>

#include "AvionicsState.h"
#include "Pi.h"
#include "AvionicsKF.h"


#define BMP_ADDR_PIN 36
#define RPI_PWR 0
#define RPI_VIDEO 1


using namespace mmfs;

Logger logger(ALTERNATING_BOTH, 25000, 300);        // 25 kB buffer, 300 write interval

BNO055 bno;         // I2C Address 0x29
BMP390 bmp;         // I2C Address 0x77
MAX_M10S gps; // I2C Address 0x42
BMI088 bmi;

//RadioSettings settings = {433.78, 0x01, 0x02, &hardware_spi, 10, 31, 32};
//RFM69HCW radio(&settings);
// APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1", '^', 'M'};
// APRSCmdData currentCmdData = {800000, 800000, 800000, false};
// APRSCmdMsg cmd(header);
// APRSTelemMsg telem(header);
// int timeOfLastCmd = 0;
// const int CMD_TIMEOUT_SEC = 100; // 10 seconds
// void processCurrentCmdData(double time);

AvionicsState *computer; // = useKalmanFilter = true, stateRecordsOwnData = true
uint32_t radioTimer = millis();
Pi rpi(RPI_PWR, RPI_VIDEO);

static double last = 0; // for better timing than "delay(100)"

// Free memory debug function
extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

void FreeMem()
{
    void *heapTop = malloc(50);
    Serial.print((long)heapTop);
    Serial.print(" ");
    free(heapTop);
}
// Free memory debug function

const int BUZZER_PIN = 33;
const int BUILTIN_LED_PIN = LED_BUILTIN;
int allowedPins[] = {BUILTIN_LED_PIN, BUZZER_PIN};
BlinkBuzz bb(allowedPins, 2, true);

const int SENSOR_BIAS_CORRECTION_DATA_LENGTH = 2;
const int SENSOR_BIAS_CORRECTION_DATA_IGNORE = 1;
const int UPDATE_RATE = 10;
const int UPDATE_INTERVAL = 1000.0 / UPDATE_RATE;


void setup()
{
    MAX_M10S gps;
    BNO055 imu;
    BMP390 baro;
    Sensor *sensors[3] = {&gps, &imu, &baro};
    AvionicsKF kfilter;
    computer = new AvionicsState(sensors, 3, &kfilter, false);

    // delay(5000);
    logger.init();

    pinMode(BMP_ADDR_PIN, OUTPUT);
    digitalWrite(BMP_ADDR_PIN, HIGH);

    logger.recordLogData(INFO_, "Initializing Avionics System. 5 second delay to prevent unnecessary file generation.", TO_USB);
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

    if (computer->init())
    {
        logger.recordLogData(INFO_, "All Sensors Initialized");
        bb.onoff(BUZZER_PIN, 1000);
    }
    else
    {
        logger.recordLogData(ERROR_, "Some Sensors Failed to Initialize. Disabling those sensors.");
        bb.onoff(BUZZER_PIN, 200, 3);
    }
    //sendSDCardHeader(computer->getCsvHeader());
}

void loop()
{
    double time = millis();
    bb.update();

    // Update the Radio
    // if (radio.update()) // if there is a message to be read
    // {
    //     timeOfLastCmd = time;
    //     APRSCmdData old = cmd.data;
    //     if (radio.dequeueReceive(&cmd))
    //         radioHandler::processCmdData(cmd, old, currentCmdData, time);
    // }
    // radioHandler::processCurrentCmdData(currentCmdData, computer, rpi, time);

    // Update the state of the rocket
    if (time - last < 100)
        return;

    last = time;
    computer->updateState();
    logger.recordLogData(INFO_, computer->getStateString(), TO_USB);
    // Send Telemetry Data
    // if (time - radioTimer >= 500)
    // {
    //     computer.fillAPRSData(telem.data);

    //     int status = PI_ON * rpi.isOn() +
    //                  PI_VIDEO * rpi.isRecording() +
    //                  RECORDING_DATA * computer.getRecordOwnFlightData();
    //     telem.data.statusFlags = status;

    //     radio.enqueueSend(&telem);
    //     radioTimer = time;
    // }

}


