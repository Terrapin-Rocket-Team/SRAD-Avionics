#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <Arduino.h>
#include "FakeBaro.h"
#include "FakeGPS.h"
#include "FakeIMU.h"

//column numbers, 0 indexed
int timeAbsoluteCol = 0;
int numColums = 17;
int AX = 3;//m/s/s
int AY = 4;
int AZ = 5;   // Z is up
int OX = 7;
int OY = 8;
int OZ = 9;
int OW = 6;
int BAlt = 10; // m
int BTemp = 12; // m
int BPres = 11; // m
int GX = 15;//lat/long
int GY = 14;
int GZ = 16;//Z is up

double ParseIncomingFakeSensorData(String line, FakeBaro& baro, FakeGPS& gps, FakeIMU& fimu){
    if(line.length() == 0) return;
    String columns[numColums];
    int cursor = 0;
    for(unsigned int j = 0;j < line.length();j++){
        if(cursor == numColums) break;
        if(line[j] ==',')
        cursor++;
        else
        columns[cursor].append(line[j]);
    }
    baro.feedData((double)columns[BAlt].toFloat(),(double)columns[BTemp].toFloat(),(double)columns[BPres].toFloat());
    gps.feedData((double)columns[GX].toFloat(), (double)columns[GY].toFloat(), (double)columns[GZ].toFloat());
    fimu.feedData((double)columns[AX].toFloat(), (double)columns[AY].toFloat(), (double)columns[AZ].toFloat(), (double)columns[OX].toFloat(), (double)columns[OY].toFloat(), (double)columns[OZ].toFloat(), (double)columns[OW].toFloat());
    return (double)columns[timeAbsoluteCol].toFloat();
}

uint32_t FreeMem(){ // for Teensy 3.0
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1000);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is (approximately) the free, available ram.
    return stackTop - heapTop;
}

#endif