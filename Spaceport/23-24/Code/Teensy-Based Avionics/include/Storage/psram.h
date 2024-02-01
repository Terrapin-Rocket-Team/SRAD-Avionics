#ifndef PSRAM_H
#define PSRAM_H

#include "sdCard.h"
#include "sdCard.cpp"
#include <vector>

#define PSRAM_DUMP_TIMEOUT .25

extern "C" uint8_t external_psram_size;
extern char *psram_memory_begin, *psram_memory_end;
extern char *psramNextLoc;  // Next open FRAM location for writing

void psramPrintln();  // Add a newline to the file in FRAM
float getPSRAMCapacity();  // % of storage used in FRAM

// Write string and newline to FRAM
template< typename T > void psramPrintln( T data ){
  String str = String(data);
  int strLen = str.length();
  for(int i = 0; i < strLen; i++){
    *psramNextLoc = str.charAt(i);
    psramNextLoc++;
  }
  *psramNextLoc = '\n';
  psramNextLoc++;
}

// Write string to FRAM
template< typename T > void psramPrint( T data ){
  String str = String(data);
  int strLen = str.length();
  
  for(int i = 0; i < strLen; i++){
    *psramNextLoc = str.charAt(i);
    psramNextLoc++;
  }
}

String PSRAMDumpToSD();  // Dump FRAM to SD Card, returns Timeout if timeout and Dumped if successful
void PSRAMPreLaunchDump();

bool isPSRAMReady();  // Returns whether the FRAM is initialized
bool isPSRAMDumped();  // Returns whether the FRAM has been dumped
char* getPSRAMNextLoc();  // Returns the next free memory location in FRAM
bool setupPSRAM(String csvHeader);  // Initializes the FRAM
void resetPSRAMDumpStatus();  // Resets PSRAM Dump Status

void psramMarkLiftoff();

void insertBlankValuesPSRAM(int numValues);  // Adds the specified number of blank values to the CSV row. Argument is # of blank values to insert

#endif