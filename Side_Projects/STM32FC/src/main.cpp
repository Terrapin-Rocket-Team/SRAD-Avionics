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

// Copy every ARC frame we send to the USB console too. Set to 0 for flight.
#define ARC_DEBUG_MIRROR 1

// Hijacked board LEDs for link diagnostics: one event per LED so heartbeat and
// telemetry are visible independently. Both assumed active-high (HIGH = lit);
// flip the writes if your board sinks current to light them.
#define HEARTBEAT_LED PE5
#define TELEM_LED PE6
#define RADIO_NRST PC13
#define RADIO_BUSY PE3
#define RADIO_NCS PA15
#define RADIO_IO8 PA3
// irq pin
#define RADIO_IO9 PE2

#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3
#define TELEMETRY_UART_TX PB13
#define TELEMETRY_UART_RX PB12
#define BATTERY_SENSE_PIN PC2_C

#include "LocalFileCommands.h"
#include "ArcNode.h"
#include "DataRadioTelem.h"

HardwareSerial TelemetryUART(TELEMETRY_UART_RX, TELEMETRY_UART_TX);

// This FC is the nosecone ARC node; ARC frames go out over the hub link.
ArcNode arcNode(ARC_ADDR_FC_N, TelemetryUART);

// Telemetry for the Terrapin ground station, in the proprietary data radio's
// format. We only build the frame here; it is wrapped in a RADIO/DATA_DOWNLINK
// message and routed over ARC to the data radio (the onboard radio is unused).
const APRSConfig kDataRadioConfig = {
    "KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
DataRadioTelem dataRadioTelem(kDataRadioConfig);
FileLogSink dataLog("data_log.csv", StorageBackend::EMMC, false);
FileLogSink eventLog("event_log.csv", StorageBackend::EMMC, false);
PrintLog serialEventLog(Serial, true);
ILogSink *eventLogSinks[] = {&serialEventLog, &eventLog};
ILogSink *dataLogSinks[] = {&dataLog};

constexpr uint32_t kTelemetryBaud = 115200;
constexpr uint32_t kArcTelemPeriodMs = 100;       // ARC flight telemetry to the hub (10 Hz)
constexpr uint32_t kArcHeartbeatPeriodMs = 1000;  // ARC heartbeat broadcast (1 Hz)
constexpr uint32_t kDataRadioPeriodMs = 1000;     // RadioMessage downlink (1 Hz)
constexpr int kBatteryDividerR1Ohms = 422000;
constexpr int kBatteryDividerR2Ohms = 102000;
constexpr float kBatteryCalibrationGain = 1.00733f;
constexpr float kBatteryCalibrationOffset = -0.064f;

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
    uint32_t lastArcTelemMs = 0;
    uint32_t lastHeartbeatMs = 0;
    uint32_t lastDataRadioMs = 0;

    // Activity-LED blinks (diagnostic): each LED idles LOW and is pulsed HIGH
    // briefly when its event fires. Heartbeat and telemetry get their own LED.
    constexpr uint32_t kHeartbeatBlinkMs = 120;
    constexpr uint32_t kTelemBlinkMs = 40;

    struct LedPulse
    {
        uint8_t pin;
        bool active;
        uint32_t offAtMs;
    };

    LedPulse heartbeatLed = {HEARTBEAT_LED, false, 0};
    LedPulse telemLed = {TELEM_LED, false, 0};

    void pulse(LedPulse &led, uint32_t durationMs)
    {
        digitalWrite(led.pin, HIGH);
        led.offAtMs = millis() + durationMs;
        led.active = true;
    }

    void serviceLed(LedPulse &led)
    {
        if (led.active && static_cast<int32_t>(millis() - led.offAtMs) >= 0)
        {
            digitalWrite(led.pin, LOW);
            led.active = false;
        }
    }

    float metersToFeet(double meters)
    {
        return static_cast<float>(meters * 3.28083989501312);
    }

    template <typename T>
    long long clampToRange(double value, T minValue, T maxValue)
    {
        const double rounded = llround(value);
        if (rounded < static_cast<double>(minValue))
            return minValue;
        if (rounded > static_cast<double>(maxValue))
            return maxValue;
        return static_cast<long long>(rounded);
    }

    int16_t toI16(double value)
    {
        return static_cast<int16_t>(clampToRange(value, INT16_MIN, INT16_MAX));
    }

    int32_t toI32(double value)
    {
        return static_cast<int32_t>(clampToRange(value, INT32_MIN, INT32_MAX));
    }

    bool trySampleBatteryVoltage(float &batteryVolts)
    {
        if (!batterySense.isInitialized() || !batterySense.isHealthy())
            return false;

        const float rawBatteryVoltage = static_cast<float>(batterySense.getVoltage());
        batteryVolts = (kBatteryCalibrationGain * rawBatteryVoltage) + kBatteryCalibrationOffset;
        return true;
    }

    // Map Astra's flight stage onto the ARC FC_COORD stage enum.
    uint8_t arcStage(astra_rocket::FlightStage stage)
    {
        using namespace astra_rocket;
        switch (stage)
        {
        case PAD_IDLE:         return ARC_FC_COORD_STAGE_PAD;
        case BOOST:            return ARC_FC_COORD_STAGE_BOOST;
        case COAST:            return ARC_FC_COORD_STAGE_COAST;
        case APOGEE:           return ARC_FC_COORD_STAGE_COAST;
        case EXPECTING_DROGUE: return ARC_FC_COORD_STAGE_COAST;
        case UNDER_DROGUE:     return ARC_FC_COORD_STAGE_DROGUE;
        case EXPECTING_MAIN:   return ARC_FC_COORD_STAGE_DROGUE;
        case UNDER_MAIN:       return ARC_FC_COORD_STAGE_MAIN;
        case LANDED:           return ARC_FC_COORD_STAGE_LANDED;
        default:               return ARC_FC_COORD_STAGE_UNKNOWN;
        }
    }

    // Convert an earth-frame orientation quaternion to roll/pitch/yaw in degrees.
    void quatToEulerDeg(const Quaternion &q, double &rollDeg, double &pitchDeg, double &yawDeg)
    {
        const double w = q.w(), x = q.x(), y = q.y(), z = q.z();

        const double sinr_cosp = 2.0 * (w * x + y * z);
        const double cosr_cosp = 1.0 - 2.0 * (x * x + y * y);
        rollDeg = atan2(sinr_cosp, cosr_cosp) * RAD_TO_DEG;

        double sinp = 2.0 * (w * y - z * x);
        sinp = sinp > 1.0 ? 1.0 : (sinp < -1.0 ? -1.0 : sinp);
        pitchDeg = asin(sinp) * RAD_TO_DEG;

        const double siny_cosp = 2.0 * (w * z + x * y);
        const double cosy_cosp = 1.0 - 2.0 * (y * y + z * z);
        yawDeg = atan2(siny_cosp, cosy_cosp) * RAD_TO_DEG;
    }

    constexpr double kMs2ToMilliG = 1000.0 / 9.80665;

    void publishArcTelemetryIfDue()
    {
        const uint32_t now = millis();
        if ((now - lastArcTelemMs) < kArcTelemPeriodMs)
            return;
        lastArcTelemMs = now;

        RocketState *state = rocket.getRocketState();
        if (state == nullptr)
            return;

        arc_fc_coord_flight_telemetry_t telem = {};
        telem.time_ms = now;
        telem.stage = arcStage(state->getFlightStage());

        const Vector<3> accel = state->getAcceleration();
        telem.accel_x_mg = toI16(accel.x() * kMs2ToMilliG);
        telem.accel_y_mg = toI16(accel.y() * kMs2ToMilliG);
        telem.accel_z_mg = toI16(accel.z() * kMs2ToMilliG);

        const Vector<3> vel = state->getVelocity();
        telem.vel_x_cms = toI16(vel.x() * 100.0);
        telem.vel_y_cms = toI16(vel.y() * 100.0);
        telem.vel_z_cms = toI16(vel.z() * 100.0);

        telem.alt_cm = toI32(state->getAltitudeAGL() * 100.0);

        double rollDeg = 0.0, pitchDeg = 0.0, yawDeg = 0.0;
        quatToEulerDeg(state->getRocketOrientation(), rollDeg, pitchDeg, yawDeg);
        telem.roll_cdeg = toI16(rollDeg * 100.0);
        telem.pitch_cdeg = toI16(pitchDeg * 100.0);
        telem.yaw_cdeg = toI16(yawDeg * 100.0);

        if (Barometer *baro = config.getSensorManager()->getBaroSource();
            baro != nullptr && baro->isInitialized())
        {
            telem.temp_cdeg = toI16(baro->getTemp() * 100.0);
        }

        float batteryVolts = 0.0f;
        if (trySampleBatteryVoltage(batteryVolts))
            telem.voltage_mv = static_cast<uint16_t>(clampToRange(batteryVolts * 1000.0, 0, UINT16_MAX));

        telem.gps_fix_quality = ARC_FC_COORD_GPS_FIX_NONE;
        if (GPS *gps = config.getSensorManager()->getGPSSource();
            gps != nullptr && gps->isInitialized())
        {
            telem.gps_fix_quality = static_cast<uint8_t>(clampToRange(gps->getFixQual(), 0, 0xFF));
            if (gps->getHasFix())
            {
                const Vector<3> pos = gps->getPos();
                telem.lat_e7 = toI32(pos.x() * 1e7);
                telem.lon_e7 = toI32(pos.y() * 1e7);
            }
        }

        if (arcNode.sendFlightTelemetry(telem))
            pulse(telemLed, kTelemBlinkMs);
    }

    void publishHeartbeatIfDue()
    {
        const uint32_t now = millis();
        if ((now - lastHeartbeatMs) < kArcHeartbeatPeriodMs)
            return;
        lastHeartbeatMs = now;
        if (arcNode.sendHeartbeat())
            pulse(heartbeatLed, kHeartbeatBlinkMs);
    }

    void publishDataRadioIfDue()
    {
        const uint32_t now = millis();
        if ((now - lastDataRadioMs) < kDataRadioPeriodMs)
            return;
        lastDataRadioMs = now;

        RocketState *state = rocket.getRocketState();
        if (state == nullptr)
            return;

        double lat = 0.0, lng = 0.0;
        uint8_t fixQual = 0;
        if (GPS *gps = config.getSensorManager()->getGPSSource();
            gps != nullptr && gps->isInitialized())
        {
            fixQual = static_cast<uint8_t>(gps->getFixQual());
            if (gps->getHasFix())
            {
                const Vector<3> pos = gps->getPos();
                lat = pos.x();
                lng = pos.y();
            }
        }

        double rollDeg = 0.0, pitchDeg = 0.0, yawDeg = 0.0;
        quatToEulerDeg(state->getRocketOrientation(), rollDeg, pitchDeg, yawDeg);
        const double orient[3] = {rollDeg, pitchDeg, yawDeg};

        const double altFt = metersToFeet(state->getAltitudeAGL());
        const double spdKnots = state->getVelocity().magnitude() * 1.943844; // m/s -> knots
        const double hdgDeg = (config.getSensorManager()->getGPSSource() != nullptr)
                                  ? config.getSensorManager()->getGPSSource()->getHeading()
                                  : 0.0;

        uint8_t tempC = 0;
        if (Barometer *baro = config.getSensorManager()->getBaroSource();
            baro != nullptr && baro->isInitialized())
        {
            tempC = static_cast<uint8_t>(clampToRange(baro->getTemp(), 0, 0x7F));
        }

        // Build the vendor frame, then route it to the data radio over ARC.
        const uint16_t n = dataRadioTelem.build(lat, lng, altFt, spdKnots, hdgDeg, orient,
                                                tempC, arcStage(state->getFlightStage()), fixQual);
        if (n > 0 && arcNode.sendDataRadioDownlink(dataRadioTelem.data(), dataRadioTelem.size()))
            pulse(telemLed, kTelemBlinkMs);
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
    }

} // namespace

void setup()
{
    pinMode(HEARTBEAT_LED, OUTPUT);
    pinMode(TELEM_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);
    digitalWrite(TELEM_LED, LOW);
    analogReadResolution(12);
    Serial.begin(115200);
    waitForSerialReady();
    EventLogger::configure(eventLogSinks, 1);
    setupTelemetryUart();

    // Copy every ARC frame we send out the USB console too (same raw bytes as
    // the hub link). Set ARC_DEBUG_MIRROR 0 to drop the USB copy for flight.
#if ARC_DEBUG_MIRROR
    arcNode.addMirror(Serial);
#endif

    LOGI("ARC node FC_N online; telemetry + data-radio downlink over UART");

    config.withFlightLogRate(50)
        .withPreflightLogRate(10)
        .withPostflightLogRate(10)
        .with6DoFIMU(&imu)
        .withBaro(&baro)
        .withGPS(&gps)
        .withMag(&mag)
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
        // Both LEDs fast-blinking together = stuck in init failure (never
        // reaches loop(), so nothing is transmitted -- a likely "hub can't see
        // it" cause).
        while (1)
        {
            digitalWrite(HEARTBEAT_LED, HIGH);
            digitalWrite(TELEM_LED, HIGH);
            delay(80);
            digitalWrite(HEARTBEAT_LED, LOW);
            digitalWrite(TELEM_LED, LOW);
            delay(80);
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
    serviceLed(heartbeatLed);
    serviceLed(telemLed);
    rocket.update();
    publishArcTelemetryIfDue();
    publishDataRadioIfDue();
    publishHeartbeatIfDue();
}
