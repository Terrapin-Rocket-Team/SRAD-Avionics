#include <Arduino.h>
#include "State.h"
#include "BMP390.h"
#include "BNO055.h"
#include "MAX_M10S.h"
#include "DS3231.h"
#include "Radio/RFM69HCW.h"
#include "Radio/APRS/APRSCmdMsg.h"
#include "Radio/APRS/APRSTelemMsg.h"
#include "RecordData.h"
#include "BlinkBuzz.h"
#include "Pi.h"

#define BMP_ADDR_PIN 36
#define RPI_PWR 0
#define RPI_VIDEO 1

BNO055 bno(13, 12);         // I2C Address 0x29
BMP390 bmp(13, 12);         // I2C Address 0x77
MAX_M10S gps(13, 12, 0x42); // I2C Address 0x42

RadioSettings settings = {915.0, 0x01, 0x02, &hardware_spi, 10, 31, 32};
RFM69HCW radio(&settings);
APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1", '^', 'M'};
APRSCmdData currentCmdData = {800000, 800000, 800000, false};
APRSCmdMsg cmd(header);
APRSTelemMsg telem(header);
int timeOfLastCmd = 0;
const int CMD_TIMEOUT_SEC = 100; // 10 seconds
void processCurrentCmdData(double time);

State computer; // = useKalmanFilter = true, stateRecordsOwnData = true
uint32_t radioTimer = millis();
Pi rpi(RPI_PWR, RPI_VIDEO);
PSRAM *ram;

static double last = 0; // for better timing than "delay(100)"

// BlinkBuzz setup
int BUZZER = 33;
int LED = LED_BUILTIN;
int allowedPins[] = {LED};
BlinkBuzz bb(allowedPins, 1, true);

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

void setup()
{

    bb.onoff(BUZZER, 100, 4, 100);
    recordLogData(INFO, "Initializing Avionics System. 5 second delay to prevent unnecessary file generation.", TO_USB);
    // delay(5000);

    pinMode(BMP_ADDR_PIN, OUTPUT);
    digitalWrite(BMP_ADDR_PIN, HIGH);
    ram = new PSRAM(); // init after the SD card for better data logging.

    // The SD card MUST be initialized first to allow proper data logging.
    if (setupSDCard())
    {

        recordLogData(INFO, "SD Card Initialized");
        bb.onoff(BUZZER, 1000);
    }
    else
    {
        recordLogData(ERROR, "SD Card Failed to Initialize");

        bb.onoff(BUZZER, 200, 3);
    }

    // The PSRAM must be initialized before the sensors to allow for proper data logging.

    if (ram->init())
        recordLogData(INFO, "PSRAM Initialized");
    else
        recordLogData(ERROR, "PSRAM Failed to Initialize");

    if (!computer.addSensor(&bmp))
        recordLogData(INFO, "Failed to add BMP390 Sensor");
    if (!computer.addSensor(&gps))
        recordLogData(INFO, "Failed to add MAX_M10S Sensor");
    if (!computer.addSensor(&bno))
        recordLogData(INFO, "Failed to add BNO055 Sensor");
    computer.setRadio(&radio);
    if (computer.init())
    {
        recordLogData(INFO, "All Sensors Initialized");
        bb.onoff(BUZZER, 1000);
    }
    else
    {
        recordLogData(ERROR, "Some Sensors Failed to Initialize. Disabling those sensors.");
        bb.onoff(BUZZER, 200, 3);
    }
    sendSDCardHeader(computer.getCsvHeader());
}

void loop()
{
    double time = millis();
    bb.update();
    if (radio.update()) // if there is a message to be read
    {
        timeOfLastCmd = time;
        APRSCmdData old = cmd.data;
        if (radio.dequeueReceive(&cmd))
        {
            char log[100];
            if (cmd.data.Launch != old.Launch)
            {
                snprintf(log, 100, "Launch Command Changed to %d.", cmd.data.Launch);
                recordLogData(INFO, log);
                currentCmdData.Launch = cmd.data.Launch;
            }
            if (cmd.data.MinutesUntilPowerOn != old.MinutesUntilPowerOn)
            {
                snprintf(log, 100, "Power On Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilPowerOn, (int)(currentCmdData.MinutesUntilPowerOn - time) / 60000);
                recordLogData(INFO, log);
                currentCmdData.MinutesUntilPowerOn = cmd.data.MinutesUntilPowerOn * 60 * 1000 + time;
            }
            if (cmd.data.MinutesUntilVideoStart != old.MinutesUntilVideoStart)
            {
                snprintf(log, 100, "Video Start Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilVideoStart, (int)(currentCmdData.MinutesUntilVideoStart - time) / 60000);
                recordLogData(INFO, log);
                currentCmdData.MinutesUntilVideoStart = cmd.data.MinutesUntilVideoStart * 60 * 1000 + time;
            }
            if (cmd.data.MinutesUntilDataRecording != old.MinutesUntilDataRecording)
            {
                snprintf(log, 100, "Data Recording Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilDataRecording, (int)(currentCmdData.MinutesUntilDataRecording - time) / 60000);
                recordLogData(INFO, log);
                currentCmdData.MinutesUntilDataRecording = cmd.data.MinutesUntilDataRecording * 60 * 1000 + time;
            }
        }
    }
    processCurrentCmdData(time);

    if (time - last < 100)
        return;

    last = time;
    computer.updateState();
    // recordLogData(INFO, computer.getStateString(), TO_USB);

    if (time - radioTimer >= 1000)
    {
        computer.fillAPRSData(telem.data);

        if (rpi.isOn())
            telem.data.statusFlags |= RPI_PWR;
        else
            telem.data.statusFlags &= ~RPI_PWR;

        if (rpi.isRecording())
            telem.data.statusFlags |= RPI_VIDEO;
        else
            telem.data.statusFlags &= ~RPI_VIDEO;

        if (computer.getRecordOwnFlightData())
            telem.data.statusFlags |= RECORDING_DATA;
        else
            telem.data.statusFlags &= ~RECORDING_DATA;

        radio.enqueueSend(&telem);
        radioTimer = time;
    }

    // RASPBERRY PI TURN ON/VIDEO
    if ((time / 1000.0 > 810 && time - timeOfLastCmd > CMD_TIMEOUT_SEC * 1000) || computer.getStageNum() >= 1)
        rpi.setOn(true);
    if ((computer.getStageNum() >= 1 || time - timeOfLastCmd > CMD_TIMEOUT_SEC * 1000) && rpi.isOn())
        rpi.setRecording(true);
}

void processCurrentCmdData(double time)
{
    if (currentCmdData.Launch && computer.getStageNum() == 0)
    {
        recordLogData(INFO, "Launch Command Received. Launching Rocket.");
        computer.launch();
    }

    if (time > currentCmdData.MinutesUntilPowerOn && !rpi.isOn())
    {
        recordLogData(INFO, "Power On RPI Time Reached. Turning on Raspberry Pi.");
        rpi.setOn(true);
    }
    if (time > currentCmdData.MinutesUntilVideoStart && !rpi.isRecording())
    {
        recordLogData(INFO, "Video Start Time Reached. Starting Video Recording.");
        rpi.setRecording(true);
    }
    if (time > currentCmdData.MinutesUntilDataRecording && !computer.getRecordOwnFlightData())
    {
        recordLogData(INFO, "Data Recording Time Reached. Starting Data Recording.");
        computer.setRecordOwnFlightData(true);
    }
}
