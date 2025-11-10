#include <Wire.h>
#include <string.h>
#include <stdint.h>

#define I2C_ADDRESS 0x10
#define CHUNK_SIZE 32
#define DATA_PER_CHUNK 29

uint8_t telemetryData[128];
int totalChunks = 0;
int currentChunk = 0;
bool dataReady = false;

void setup() {
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(sendTelemetryChunk);
  Wire.onReceive(receiveCommand);
  
  // Calculate total chunks needed for telemetry data
  totalChunks = (sizeof(telemetryData) + DATA_PER_CHUNK - 1) / DATA_PER_CHUNK;
  currentChunk = 0;
  dataReady = false;
}

// get telemetry data from sensors
void loop(){
    for (int i = 0; i < sizeof(telemetryData); i++) {
        telemetryData[i] = i & 0xFF;
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