#include <Arduino.h>

#include <Sensors/HW/Baro/DPS368.h>
#include <Sensors/HW/GPS/SAM_M10Q.h>
#include <Sensors/VoltageSensor/VoltageSensor.h>

#include <Utils/Astra.h>
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
#define BATTERY_SENSE_PIN PC2_C

#include "LocalFileCommands.h"
#include "AvionicsPacketProtocol.h"
#include "PacketTransports.h"
#include "Type_2GT.h"

using namespace astra;

SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);
PacketTransports packetTransports;
FileLogSink dataLog("data_log.csv", StorageBackend::EMMC, false);
FileLogSink eventLog("event_log.csv", StorageBackend::EMMC, false);
PrintLog serialEventLog(Serial, true);
ILogSink *eventLogSinks[] = {&serialEventLog, &eventLog};
ILogSink *dataLogSinks[] = {&dataLog};

constexpr uint32_t kBppTelemPeriodMs = 500;
constexpr int kBatteryDividerR1Ohms = 422000;
constexpr int kBatteryDividerR2Ohms = 102000;
constexpr float kBatteryCalibrationGain = 1.00733f;
constexpr float kBatteryCalibrationOffset = -0.064f;

void radInt(void)
{
    radio.respondToIrq();
    digitalWrite(STATUS_LED, LOW);
}

SAM_M10Q gps;
DPS368 baro;
VoltageSensor batterySense(BATTERY_SENSE_PIN, kBatteryDividerR1Ohms, kBatteryDividerR2Ohms, "Battery Voltage");

AstraConfig config;
Astra sys(&config);

namespace
{
    uint32_t lastBppTelemMs = 0;
    constexpr uint32_t kVelocityResetThresholdMs = 5000;

    struct DerivedVelocitySample
    {
        bool hasSample = false;
        double altitudeM = 0.0;
        uint32_t timeMs = 0;
    };

    DerivedVelocitySample lastBaroVelocitySample;
    DerivedVelocitySample lastGpsVelocitySample;

    float metersToFeet(double meters)
    {
        return static_cast<float>(meters * 3.28083989501312);
    }

    bool trySampleBatteryVoltage(float &batteryVolts)
    {
        if (!batterySense.isInitialized() || !batterySense.isHealthy())
            return false;

        const float rawBatteryVoltage = static_cast<float>(batterySense.getVoltage());
        batteryVolts = rawBatteryVoltage;
        return true;
    }

    bool updateDerivedVelocity(
        double altitudeM,
        uint32_t nowMs,
        DerivedVelocitySample &state,
        float &velocityZMs)
    {
        if (!std::isfinite(altitudeM))
            return false;

        bool hasVelocity = false;
        if (state.hasSample)
        {
            const uint32_t dtMs = nowMs - state.timeMs;
            if (dtMs > 0 && dtMs <= kVelocityResetThresholdMs)
            {
                const double dtSeconds = static_cast<double>(dtMs) / 1000.0;
                velocityZMs = static_cast<float>((altitudeM - state.altitudeM) / dtSeconds);
                hasVelocity = true;
            }
        }

        state.hasSample = true;
        state.altitudeM = altitudeM;
        state.timeMs = nowMs;
        return hasVelocity;
    }

    bool buildBppTelemetryPacket(avionics_packet::PacketBuffer &packet)
    {
        avionics_packet::BppTelemetry telemetry = {};
        const uint32_t nowMs = millis();

        if (baro.isInitialized() && baro.isHealthy())
        {
            const double baroAltitudeM = baro.getASLAltM();
            telemetry.hasBaroAltitude = true;
            telemetry.baroAltitudeFeet = metersToFeet(baroAltitudeM);

            float baroVelocityZMs = 0.0f;
            telemetry.hasBaroVelocity = updateDerivedVelocity(
                baroAltitudeM,
                nowMs,
                lastBaroVelocitySample,
                baroVelocityZMs);
            telemetry.baroVelocityZMs = baroVelocityZMs;
        }

        float batteryVolts = 0.0f;
        telemetry.hasBattery = trySampleBatteryVoltage(batteryVolts);
        telemetry.batteryVolts = batteryVolts;

        if (gps.isInitialized() && gps.isHealthy() && gps.getHasFix())
        {
            const Vector<3> gpsPos = gps.getPos();
            const double gpsAltitudeM = gpsPos.z();
            telemetry.hasGps = true;
            telemetry.latitudeDeg = gpsPos.x();
            telemetry.longitudeDeg = gpsPos.y();
            telemetry.gpsAltitudeFeet = metersToFeet(gpsAltitudeM);

            float gpsVelocityZMs = 0.0f;
            telemetry.hasGpsVelocity = updateDerivedVelocity(
                gpsAltitudeM,
                nowMs,
                lastGpsVelocitySample,
                gpsVelocityZMs);
            telemetry.gpsVelocityZMs = gpsVelocityZMs;
        }

        return avionics_packet::encodeBppTelemetry(telemetry, packet);
    }

    void publishBppTelemetryIfDue()
    {
        const uint32_t now = millis();
        if ((now - lastBppTelemMs) < kBppTelemPeriodMs)
            return;

        lastBppTelemMs = now;

        avionics_packet::PacketBuffer packet;
        if (!buildBppTelemetryPacket(packet))
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

} // namespace

void setup()
{
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    analogReadResolution(16);
    pinMode(BATTERY_SENSE_PIN, INPUT_ANALOG);
    Serial.begin(115200);
    waitForSerialReady();
    EventLogger::configure(eventLogSinks, 1);
    packetTransports.setRadio(&radio);
    int rc = radio.begin();
    if (rc != RADIOLIB_ERR_NONE)
    {
        LOGE("Radio initialization failed, code: %d", rc);
    }
    else
    {
        LOGI("Own radio enabled for BPP telemetry");
        radio.onIrq(radInt);
        radio.recieve();
    }

    config.withLoggingRate(10)
        .withBaro(&baro)
        .withGPS(&gps)
        .withEventLogs(eventLogSinks, 2)
        .withDataLogs(dataLogSinks, 1)
        .withName("STM32FC-BPP")
        .withMiscSensor(&batterySense);

    const int initErrors = sys.init();
    if (initErrors != 0)
    {
        Serial.print("WARN: Astra initialized with ");
        Serial.print(initErrors);
        Serial.println(" sensor error(s)");
    }

    // This Astra branch configures VoltageSensor pins as plain INPUT during begin(),
    // so restore analog mode for the battery sense channel after init completes.
    pinMode(BATTERY_SENSE_PIN, INPUT_ANALOG);

    Serial.println("Astra BPP initialized");

    if (SerialMessageRouter *router = sys.getMessageRouter())
    {
        stm32fc::registerLocalFileCommands(*router);
    }
    else
    {
        Serial.println("ERR: Astra router unavailable");
    }
}

void loop()
{
    radio.service();
    if (radio.hasData())
    {
        char str[256];
        radio.readData(str, sizeof(str));
        radio.recieve();
    }

    sys.update();
    publishBppTelemetryIfDue();
}
