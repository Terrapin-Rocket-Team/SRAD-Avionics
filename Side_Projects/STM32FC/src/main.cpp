#include <Arduino.h>

#include <Sensors/HW/IMU/BMI088.h>
#include <Sensors/HW/Baro/DPS368.h>
#include <Sensors/HW/Accel/H3LIS331DL.h>
#include <Sensors/HW/GPS/SAM_M10Q.h>
#include <Sensors/HW/Mag/MMC5603NJ.h>
#include <Sensors/VoltageSensor/VoltageSensor.h>

#include <AstraRocket.h>
#include <RecordData/Logging/EventLogger.h>
#include <RecordData/Logging/LoggingBackend/ILogSink.h>

#define STATUS_LED PC0
#define RADIO_NRST PC13
#define RADIO_BUSY PE3
#define RADIO_NCS PA15
#define RADIO_IO8 PA3
// irq pin
#define RADIO_IO9 PE2

#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3
#define TELEMETRY_UART_TX PB12
#define TELEMETRY_UART_RX PB13
#define BATTERY_SENSE_PIN PC2_C

#include "LocalFileCommands.h"
#include "AvionicsPacketProtocol.h"
#include "PacketTransports.h"
#include "Type_2GT.h"

SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);
HardwareSerial TelemetryUART(TELEMETRY_UART_RX, TELEMETRY_UART_TX);
PacketTransports packetTransports;
FileLogSink dataLog("data_log.csv", StorageBackend::EMMC, false);
FileLogSink eventLog("event_log.csv", StorageBackend::EMMC, false);
PrintLog serialEventLog(Serial, true);
ILogSink *eventLogSinks[] = {&serialEventLog, &eventLog};
ILogSink *dataLogSinks[] = {&dataLog};

constexpr uint32_t kTelemetryBaud = 115200;
constexpr uint32_t kAviTelemPeriodMs = 500;
constexpr int kBatteryDividerR1Ohms = 422000;
constexpr int kBatteryDividerR2Ohms = 102000;
constexpr float kBatteryCalibrationGain = 1.00733f;
constexpr float kBatteryCalibrationOffset = -0.064f;

void radInt(void)
{
    radio.respondToIrq();
    digitalWrite(STATUS_LED, LOW);
}

using namespace astra;
using namespace astra_rocket;

SAM_M10Q gps;
DPS368 baro;
BMI088 imu;
H3LIS331DL highGAccel(&Wire, 0x19);
MMC5603NJ mag;
VoltageSensor batterySense(BATTERY_SENSE_PIN, kBatteryDividerR1Ohms, kBatteryDividerR2Ohms, "Battery Voltage");

AstraRocketConfig config;

AstraRocket rocket(config);

namespace
{
    uint32_t lastAviTelemMs = 0;

    float metersToFeet(double meters)
    {
        return static_cast<float>(meters * 3.28083989501312);
    }

    bool trySampleBatteryVoltage(float &batteryVolts)
    {
        if (!batterySense.isInitialized() || !batterySense.isHealthy())
            return false;

        const float rawBatteryVoltage = static_cast<float>(batterySense.getVoltage());
        batteryVolts = (kBatteryCalibrationGain * rawBatteryVoltage) + kBatteryCalibrationOffset;
        return true;
    }

    bool buildAviTelemetryPacket(avionics_packet::PacketBuffer &packet)
    {
        RocketState *state = rocket.getRocketState();
        if (state == nullptr)
            return false;

        avionics_packet::AviTelemetry telemetry = {};

        telemetry.positionZFeet = metersToFeet(state->getAltitudeAGL());

        const Vector<3> velocity = state->getVelocity();
        telemetry.velocityZMs = static_cast<float>(velocity.z());

        const Vector<3> acceleration = state->getAcceleration();
        telemetry.accelZMs2 = static_cast<float>(acceleration.z());

        if (Barometer *baroSource = config.getSensorManager()->getBaroSource();
            baroSource != nullptr && baroSource->isInitialized())
        {
            telemetry.hasBaroAgl = true;
            telemetry.baroAglFeet = metersToFeet(baroSource->getASLAltM() - state->getGroundLevelMSL());
        }

        const Quaternion orientation = state->getRocketOrientation();
        telemetry.quatW = static_cast<float>(orientation.w());
        telemetry.quatX = static_cast<float>(orientation.x());
        telemetry.quatY = static_cast<float>(orientation.y());
        telemetry.quatZ = static_cast<float>(orientation.z());

        float batteryVolts = 0.0f;
        telemetry.hasBattery = trySampleBatteryVoltage(batteryVolts);
        telemetry.batteryVolts = batteryVolts;

        if (GPS *gpsSource = config.getSensorManager()->getGPSSource();
            gpsSource != nullptr && gpsSource->isInitialized() && gpsSource->getHasFix())
        {
            const Vector<3> gpsPos = gpsSource->getPos();
            telemetry.hasGps = true;
            telemetry.latitudeDeg = gpsPos.x();
            telemetry.longitudeDeg = gpsPos.y();
        }

        return avionics_packet::encodeAviTelemetry(telemetry, packet);
    }

    void publishAviTelemetryIfDue()
    {
        const uint32_t now = millis();
        if ((now - lastAviTelemMs) < kAviTelemPeriodMs)
            return;

        lastAviTelemMs = now;

        avionics_packet::PacketBuffer packet;
        if (!buildAviTelemetryPacket(packet))
            return;

        packetTransports.send(packet);
    }

    void waitForSerialReady()
    {
        const uint32_t start = millis();
        while (!Serial && (millis() - start) < 2000)
        {
            delay(10);
        }
    }

    void setupTelemetryUart()
    {
        TelemetryUART.begin(kTelemetryBaud);
        delay(100);
        packetTransports.addStream(TelemetryUART);
    }

} // namespace

void setup()
{
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    analogReadResolution(12);
    Serial.begin(115200);
    waitForSerialReady();
    EventLogger::configure(eventLogSinks, 1);
    setupTelemetryUart();
#ifdef USE_OWN_RADIO
    packetTransports.setRadio(&radio);
    int rc = radio.begin();
    if (rc != RADIOLIB_ERR_NONE)
    {
        LOGE("Radio initialization failed, code: %d", rc);
    }
    else
    {
        LOGI("Own radio enabled for AviTelem");
        radio.onIrq(radInt);
        radio.recieve();
    }
#else
    LOGI("Own radio disabled; AviTelem will use UART only");
#endif

    config.withFlightLogRate(20)
        .withPreflightLogRate(20)
        .withPostflightLogRate(20)
        .with6DoFIMU(&imu)
        .withBaro(&baro)
        .withGPS(&gps)
        .withMag(&mag)
        .withLoggingRate(20)
        .withEventLogs(eventLogSinks, 2)
        .withDataLogs(dataLogSinks, 1)
        .withName("STM32FC")
        .withMiscSensor(&highGAccel)
        .withMiscSensor(&batterySense);

    MountingTransform bmiMount =
        MountingTransform(MountingOrientation::ROTATE_NEG90_Z)
            .compose(MountingOrientation::FLIP_YZ);

    imu.setMountingTransform(bmiMount);

    mag.setMountingOrientation(MountingOrientation::IDENTITY);

    const bool initOk = rocket.init();
    if (!initOk)
    {
        Serial.println("ERR: AstraRocket init failed");
        Serial.println("ERR: See LOG/ messages above for the root cause");
        while (1)
        {
            delay(100);
        }
    }

    pinMode(BATTERY_SENSE_PIN, INPUT_ANALOG);

    Serial.println("AstraRocket initialized");

    if (Astra *sys = rocket.getAstraSystem())
    {
        if (SerialMessageRouter *router = sys->getMessageRouter())
        {
            router->withInterface(&TelemetryUART);
            stm32fc::registerLocalFileCommands(*router);
        }
        else
        {
            Serial.println("ERR: Astra router unavailable");
        }
    }
    else
    {
        Serial.println("ERR: Astra system unavailable");
    }
}

double last = 0;

void loop()
{
#ifdef USE_OWN_RADIO
    if (radio.hasData())
    {
        char str[256];
        radio.readData(str, sizeof(str));
        radio.recieve();
    }
#endif

    rocket.update();
    publishAviTelemetryIfDue();
}
