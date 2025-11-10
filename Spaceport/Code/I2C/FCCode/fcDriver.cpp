#include <Wire.h>
#include <string.h>
#include <stdint.h>
#include <Arduino.h>

// Include Astra library to access sensor types (latest structure)
#include "Sensors/GPS/MAX_M10S.h"
#include "Sensors/Baro/DPS368.h"
#include "Sensors/Accel/BMI088Accel.h"
#include "Sensors/Gyro/BMI088Gyro.h"
#include "State/State.h"
using namespace astra;

// Forward declarations - these will be linked from main.cpp
// If fcDriver.cpp is in the same project/build, these extern declarations
// will allow access to the global sensor objects defined in main.cpp
extern MAX_M10S g;               // GPS sensor (renamed from 'm' to 'g' in latest version)
extern DPS368 baro;              // Barometer sensor (renamed from 'd' to 'baro')
extern BMI088Accel acc;          // Accelerometer sensor (separated from combined IMU)
extern BMI088Gyro gyro;         // Gyroscope sensor (separated from combined IMU)
extern State avionicsState;      // State (renamed from 'AvionicsState t' to 'State avionicsState')

#define I2C_ADDRESS 0x10
#define CHUNK_SIZE 32
#define DATA_PER_CHUNK 29

// Telemetry data structure - matches the data format from main flight computer
struct TelemetryPacket {
    uint32_t timestamp;        // 4 bytes - milliseconds since boot
    float latitude;            // 4 bytes - GPS latitude
    float longitude;           // 4 bytes - GPS longitude
    float altitude;            // 4 bytes - AGL altitude in feet
    float verticalVelocity;    // 4 bytes - vertical velocity in ft/s
    float heading;             // 4 bytes - GPS heading
    float angularVelX;         // 4 bytes - IMU angular velocity X
    float angularVelY;         // 4 bytes - IMU angular velocity Y
    float angularVelZ;         // 4 bytes - IMU angular velocity Z
    float temperature;         // 4 bytes - barometer temperature
    uint8_t stage;             // 1 byte - flight stage (0-6)
    uint8_t gpsFixQuality;     // 1 byte - GPS fix quality
    uint8_t reserved[2];       // 2 bytes - padding for alignment
    // Total: 44 bytes
};

uint8_t telemetryData[128];
int totalChunks = 0;
int currentChunk = 0;
bool dataReady = false;
uint32_t lastUpdateTime = 0;
const uint32_t UPDATE_INTERVAL = 100; // Update every 100ms (10Hz)

void setup() {
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(sendTelemetryChunk);
  Wire.onReceive(receiveCommand);
  
  // Calculate total chunks needed for telemetry data
  totalChunks = (sizeof(telemetryData) + DATA_PER_CHUNK - 1) / DATA_PER_CHUNK;
  currentChunk = 0;
  dataReady = false;
}

// Pack telemetry structure into byte array
void packTelemetryData(const TelemetryPacket* telem) {
    uint8_t* ptr = telemetryData;
    
    // Copy the structure directly (it's 44 bytes)
    memcpy(ptr, telem, sizeof(TelemetryPacket));
    ptr += sizeof(TelemetryPacket);
    
    // Optionally add additional data to fill remaining space (128 - 44 = 84 bytes)
    // For example, you could add raw sensor readings, status flags, etc.
    // For now, zero out the rest
    memset(ptr, 0, 128 - sizeof(TelemetryPacket));
}

// Get telemetry data from sensors
// NOTE: If fcDriver runs on the same microcontroller as main.cpp, you can:
// 1. Include the sensor headers and access them directly
// 2. Or use a shared memory/global structure
// 3. Or receive data via Serial/SPI from the main flight computer
void loop(){
    uint32_t currentTime = millis();
    
    // Update telemetry at specified interval
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // Create telemetry packet
        TelemetryPacket telem;
        
        // Access sensor data from main.cpp using extern declarations
        // These sensor objects are defined globally in main.cpp and updated by Astra system
        telem.timestamp = currentTime;
        
        // GPS data from MAX_M10S sensor (renamed from 'm' to 'g')
        telem.latitude = (float)g.getPos().x();
        telem.longitude = (float)g.getPos().y();
        // Note: getHeading() and getFixQual() may need to be checked - using getHasFix() as boolean alternative
        telem.heading = 0.0f;  // TODO: Check if heading method exists in latest version
        telem.gpsFixQuality = g.getHasFix() ? 1 : 0;  // Using getHasFix() as boolean
        
        // Barometer data from DPS368 sensor (renamed from 'd' to 'baro')
        // Note: Latest version uses getASLAltFt() instead of getAGLAltFt()
        // You may need to subtract startAlt if using AGL altitude
        telem.altitude = (float)baro.getASLAltFt();
        // Note: getTemp() method may need to be verified
        telem.temperature = 25.0f;  // TODO: Check if baro.getTemp() exists in latest version
        
        // IMU data - now separated into accelerometer and gyroscope
        // Gyroscope angular velocity (renamed from 'b' to 'gyro')
        // Note: Method names may differ - checking for getAngularVelocity() or similar
        telem.angularVelX = 0.0f;  // TODO: Check gyro.getAngularVelocity().x() or gyro.getGyro().x()
        telem.angularVelY = 0.0f;  // TODO: Check gyro.getAngularVelocity().y() or gyro.getGyro().y()
        telem.angularVelZ = 0.0f;  // TODO: Check gyro.getAngularVelocity().z() or gyro.getGyro().z()
        
        // Processed state data from State (includes Kalman-filtered velocity)
        // Note: Latest version uses avionicsState.getVelocity().magnitude() (commented in main.cpp)
        // May need to check if getVelocity().z() exists or use magnitude()
        telem.verticalVelocity = 0.0f;  // TODO: Check avionicsState.getVelocity().z() or similar
        telem.stage = 0;  // TODO: Check if avionicsState.getStage() exists in State class
        
        telem.reserved[0] = 0;
        telem.reserved[1] = 0;
        
        // Pack into byte array
        packTelemetryData(&telem);
    }
}

void sendTelemetryChunk(){
    if (!dataReady) {
        // If no command received yet, send empty response
        Wire.write(0);
        return;
    }
    
    uint8_t packet[CHUNK_SIZE];
    int offset = currentChunk * DATA_PER_CHUNK;
    int remaining = sizeof(telemetryData) - offset;
    int len = min(DATA_PER_CHUNK, remaining);

    packet[0] = currentChunk;
    packet[1] = totalChunks;
    packet[2] = len;

    memcpy(packet + 3, telemetryData + offset, len);

    Wire.write(packet, len + 3);

    currentChunk++;
    if (currentChunk >= totalChunks) {
        currentChunk = 0;
        dataReady = false; // Reset after sending all chunks
    }
}

void receiveCommand(){
    while (Wire.available()){
        uint8_t cmd = Wire.read();
        if (cmd == 0x01) {
            currentChunk = 0;
            dataReady = true; // Mark that we're ready to send data
        }
    }
}