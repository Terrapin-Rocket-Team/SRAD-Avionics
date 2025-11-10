#include <Wire.h>
#include <string.h>
#include <stdint.h>
#include <Arduino.h>

// Include Astra library to access sensor types (latest structure from esp32gs branch)
#include "Sensors/GPS/MAX_M10S.h"
#include "Sensors/Baro/DPS368.h"
#include "Sensors/Accel/BMI088Accel.h"
#include "Sensors/Gyro/BMI088Gyro.h"
#include "State/State.h"
using namespace astra;

// Forward declarations - these will be linked from main.cpp
// If fcDriver.cpp is in the same project/build, these extern declarations
// will allow access to the global sensor objects defined in main.cpp
extern MAX_M10S g;               
extern DPS368 baro;              
extern BMI088Accel acc;          
extern BMI088Gyro gyro;         
extern State avionicsState;      

#define I2C_ADDRESS 0x10
#define CHUNK_SIZE 32
#define DATA_PER_CHUNK 29

// Telemetry data structure - matches the data format from main flight computer
struct TelemetryPacket {
    uint32_t timestamp;        // 4 bytes 
    float latitude;            // 4 bytes 
    float longitude;           // 4 bytes 
    float altitude;            // 4 bytes 
    float verticalVelocity;    // 4 bytes 
    float heading;             // 4 bytes 
    float angularVelX;         // 4 bytes 
    float angularVelY;         // 4 bytes 
    float angularVelZ;         // 4 bytes 
    float temperature;         // 4 bytes 
    uint8_t stage;             // 1 byte 
    uint8_t gpsFixQuality;     // 1 byte 
    uint8_t reserved[2];       // 2 bytes
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
        
        // GPS data from MAX_M10S sensor (variable name 'g' in latest version)
        telem.latitude = (float)g.getPos().x();
        telem.longitude = (float)g.getPos().y();
        telem.heading = 0.0f;  // TODO: Check if g.getHeading() exists - may need to compute from velocity
        telem.gpsFixQuality = g.getHasFix() ? 1 : 0;  // Using getHasFix() boolean (seen in latest main.cpp)
        
        // Barometer data from DPS368 sensor (variable name 'baro' in latest version)
        // Note: Latest version uses getASLAltFt() - you may need to subtract startAlt for AGL
        telem.altitude = (float)baro.getASLAltFt();
        // Note: Temperature method may differ - check baro.getTemp() or similar
        telem.temperature = 25.0f;  // TODO: Verify baro.getTemp() method exists
        
        // IMU data - now separated into accelerometer and gyroscope
        // Gyroscope angular velocity (variable name 'gyro' in latest version)
        // Common patterns: getGyro(), getAngularVelocity(), or getGyroData()
        telem.angularVelX = 0.0f;  // TODO: Check gyro.getGyro().x() or gyro.getAngularVelocity().x()
        telem.angularVelY = 0.0f;  // TODO: Check gyro.getGyro().y() or gyro.getAngularVelocity().y()
        telem.angularVelZ = 0.0f;  // TODO: Check gyro.getGyro().z() or gyro.getAngularVelocity().z()
        
        // Processed state data from State (includes Kalman-filtered velocity)
        // Latest version shows: avionicsState.getVelocity().magnitude() (commented out)
        // May need to check if getVelocity().z() exists or use magnitude()
        telem.verticalVelocity = 0.0f;  // TODO: Check avionicsState.getVelocity().z() or .magnitude()
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