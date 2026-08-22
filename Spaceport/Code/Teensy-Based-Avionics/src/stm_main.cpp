//Created by Divyansh Srivastava of 6/16/2026

#include <Arduino.h>
#include <AstraRocket.h>
#include <Sensors/HW/GPS/SAM_M10Q.h>
#include <Sensors/HW/Baro/MS5611.h>
#include <Sensors/HW/IMU/BMI088.h>
#include <Sensors/HW/Mag/MMC5603NJ.h>
#include <Sensors/HW/Accel/H3LIS331DL.h>
#include <Sensors/VoltageSensor/VoltageSensor.h>
#include "RadioMessage.h"


//#include "Pi.h"

#define RPI_PWR 1
#define RPI_VIDEO 0

using namespace astra;
using namespace astra_rocket;

// =================== Sensors ===================
// Primary flight sensors - registered with AstraRocket for state estimation.
// AstraRocket polls these every loop and feeds data into the Kalman filter
// and Mahony AHRS automatically.
SAM_M10Q gps;                                        // GPS
astra::MS5611 baro;                                         // Barometer
BMI088 imu;                                          // 6DoF IMU (accel + gyro)
MMC5603NJ mag;                                       // Magnetometer
H3LIS331DL highGAccel;                               // High-g accelerometer for motor burn
VoltageSensor vsfc(A0, 330, 220, "Flight Computer Voltage"); // Battery voltage monitor

// =================== AstraRocket ===================
// AstraRocket wraps Astra and handles sensor polling, Kalman filter,
// Mahony AHRS, flight stage detection, and file logging automatically.
// ENV_STM build flag (set in platformio.ini) causes AstraRocket to use
// FileLogSink for data.csv and events.log on the STM32 storage backend.
AstraRocketConfig rocketConfig;
AstraRocket rocket(rocketConfig);

// =================== Radio ===================
// APRS packets are encoded on this STM32 and sent to a separate radio STM32
// via UART on Serial1. The radio STM32 handles RF transmission.
// Serial1 is also listened to for incoming packets from the radio STM32
// (e.g. ground station commands).
// TODO: confirm Serial1 is the correct UART port and baud rate
APRSConfig aprsConfigAvionics = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
// stateFlags encoding: 7 bits = temperature, 4 bits = flight stage, 4 bits = GPS fix quality
// packed into the 5 base91 character stateFlags field at the end of the APRS packet
uint8_t encoding[] = {7, 4, 4};

// NOTE: Airbrake comms are not used on this FC but kept here for reference if needed in future
// APRSConfig aprsConfigAirbrake = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
// Message msgAirbrake;
// bool sendAirbrake = false;

// =================== Pi Camera ===================
// Raspberry Pi camera control - powers on at 44 minutes and starts
// recording when liftoff is detected
//Pi pi(RPI_PWR, RPI_VIDEO);

void setup()
{
    // ---- Sensor Registration ----
    // Register all sensors with AstraRocket before init.
    // AstraRocket will initialize them and wire them into the
    // Kalman filter and Mahony AHRS automatically.
    rocketConfig
    .withStorageBackend(StorageBackend::SD_CARD) //ISSUE WITH THE BRANCH ASTRA ROCKET HAS, MUST CHECK
    .withGPS(&gps)
    .withBaro(&baro)
    .with6DoFIMU(&imu)
    .withMag(&mag)
    .withMiscSensor(&highGAccel)
    .withMiscSensor(&vsfc);

    // ---- AstraRocket Init ----
    // Initializes all sensors, creates RocketState with DefaultKalmanFilter
    // and MahonyAHRS, establishes baro ground level reference, and sets up
    // FileLogSink logging (data.csv + events.log) via ENV_STM build flag.
    rocket.init();

    // ---- Radio UART ----
    // Open UART to radio STM32
    // TODO: confirm baud rate for radio STM32 UART
    Serial1.begin(115200);

    LOGI("Initialization Complete");
}

uint32_t avionicsTimer = millis();

void loop()
{
    double timeeee = millis();

    // ---- Pi Camera Power ---- MUST CHECK THIS 
    // Power on Pi camera at 44 minutes (pre-apogee buffer)
    // if (millis() > 44 * 1000 * 60 && !pi.isOn())
    //     pi.setOn(true);

    // ---- AstraRocket Update ----
    // Polls all sensors, runs Mahony orientation update, runs Kalman filter
    // prediction, performs GPS/baro measurement updates, handles flight stage
    // detection, and writes to log files at the configured logging rate.
    rocket.update();

    RocketState *state = rocket.getRocketState();

    // ---- Pi Camera Recording ---- 
    // Start recording once liftoff is detected
    // MUST CHECK THIS, IT IS DIFFERENT FROM MMFS LOGIC
    //if (!pi.isRecording() && state && state->getFlightStage() > FlightStage::PAD_IDLE)
       // pi.setRecording(true);

    // ---- Incoming Radio UART ----
    // Listen for packets from radio STM32 (e.g. ground station commands)
    if (Serial1.available())
    {
        char buf[100];
        int i = Serial1.readBytes(buf, 100);
        // TODO: handle incoming ground station commands
    }

    // NOTE: Airbrake receive and rebroadcast logic removed - this FC does not communicate
    // with the airbrake board. Re-enable if airbrake comms are added in future.

    // ---- Outgoing APRS Telemetry ----
    // Encode and send avionics APRS packet to radio STM32 every 1 second.
    // Full packet format: "!MYYYYXXXX^ hhssaaoooooofffff"
    //   aprsConfigAvionics = callsign, path, message type, overlay, symbol (header)
    //   lat/lng            = GPS position
    //   alt                = barometric altitude AGL (ft)
    //   spd                = vertical velocity (ft/s)
    //   hdg                = GPS heading
    //   orient             = IMU angular velocity (x, y, z)
    //   stateFlags         = packed: temp (7 bits) | flight stage (4 bits) | GPS fix quality (4 bits)
    if (millis() - avionicsTimer > 1000)
    {
        avionicsTimer = millis();

     double orient[3] = { //ROTATABLE SENSOR, MUST CHECK 
        imu.getAngVel().x(),
        imu.getAngVel().y(),
        imu.getAngVel().z()
        };

        // Step 1: construct with stateFlags = 0 (not packed yet)
       APRSTelem aprs(
            aprsConfigAvionics,
            gps.getPos().x(),                               // raw GPS lat
            gps.getPos().y(),                               // raw GPS lng
            state ? state->getAltitudeAGL() * 3.28084 : 0.0, // filtered alt (KF) in ft
            state ? state->getVelocity().z() * 3.28084 : 0.0, // filtered velocity in ft/s
            gps.getHeading(),                               // raw GPS heading
            orient,
            0
            );

        // Step 2: define bit layout for stateFlags (7 bits temp, 4 bits stage, 4 bits fix)
        aprs.stateFlags.setEncoding(encoding, 3);

        // Step 3: pack real values into stateFlags bit fields
        uint8_t arr[] = {
            (uint8_t)(int)baro.getTemp(),
            (uint8_t)(state ? (int)state->getFlightStage() : 0),
            (uint8_t)gps.getFixQual()
        };
        aprs.stateFlags.pack(arr);

        // Step 4: encode full APRS packet (header + data + stateFlags) into buffer
        uint8_t buf[256];
        uint16_t len = aprs.encode(buf, sizeof(buf));
        if (len > 0)
        {
            // Step 5: send to radio STM32 via UART with null terminator
            Serial1.write(buf, len);
            Serial1.print('\0'); // null terminator signals end of packet to radio STM32
        }
        else
        {
            LOGE("APRS encode failed");
        }

        // NOTE: Airbrake APRS rebroadcast ~300ms after avionics APRS - removed, kept for reference
        // sendAirbrake = true;
    }

    // NOTE: Airbrake APRS rebroadcast ~300ms after avionics APRS - removed, kept for reference
    // if (sendAirbrake && millis() - avionicsTimer > 300 && millis() - avionicsTimer < 400)
    // {
    //     sendAirbrake = false;
    //     Serial1.write(msgAirbrake.buf, msgAirbrake.size);
    // }

    // ---- Loop Time Monitor ----
    // Warn if loop takes longer than 30ms - may indicate sensor or logging bottleneck
    if (millis() - timeeee > 30)
        LOGW("Loop time exceeded 30ms: %d ms", (int)(millis() - timeeee));
}