#include "psram.h"

char *psramNextLoc = 0;
char *psram_memory_begin = NULL, *psram_memory_end = NULL;
bool PSRAMDumped = false;
bool PSRAMReady = false;

char *psram_memory_cur = NULL;

std::vector<String> previousRowsPSRAM = {};
bool isLaunchedPSRAM = false;

bool setupPSRAM(String csvHeader){
  uint8_t size = external_psram_size;
  psram_memory_begin = (char *)(0x70000000);
  psram_memory_end = psram_memory_begin + (size * 1048576);
  psramNextLoc = psram_memory_begin;
  psram_memory_cur = psram_memory_begin;
  
  if(size > 0){
    // Serial.println(F("PSRAM Ready"));
    PSRAMReady = true;
    psramPrintln(csvHeader);
  }else{
    // Serial.println(F("PSRAM not found"));
    return false;
  }

  return true;
}

// Write newline to FRAM
void psramPrintln(){
  *psramNextLoc = '\n';
  psramNextLoc++;
}

// Dump FRAM to SD Card
String PSRAMDumpToSD(){
  String success;
  // Serial.println(" ");
  if(isSDReady() && PSRAMReady){
    //   Serial.print("Dumping to SD...");
      String curStr = "";
      float startTime = micros() / (1000000.0f);
      for(; psram_memory_cur < psramNextLoc; psram_memory_cur++){
        char nextByte = *psram_memory_cur;
        curStr = curStr + nextByte;
        if(nextByte == '\n'){
          logFile = sd.open(logFileName, FILE_WRITE);
          if (logFile) {
            logFile.print(curStr);
            logFile.close(); // close the file
          }
          if(!isLaunchedPSRAM){
            if(previousRowsPSRAM.size() >= COMPACT_WALKBACK_COUNT){
              previousRowsPSRAM.erase(previousRowsPSRAM.begin());
            }
            previousRowsPSRAM.push_back(String(curStr));
          }
          curStr = "";
        }

        float curTime = micros() / (1000000.0f);
        if((curTime - startTime) > PSRAM_DUMP_TIMEOUT){
          // Serial.println("SD Timeout");
          return "Timeout";
        }
      }
      // Serial.println("Dumped");
  }

  psramNextLoc = psram_memory_begin;
  PSRAMDumped = true;
  psram_memory_cur = psram_memory_begin;
  return "Dumped";
}

void PSRAMPreLaunchDump(){
  if(PSRAMReady){
      String curStr = "";
      for(char *i = psram_memory_begin; i < psramNextLoc; i++){
        char nextByte = *i;
        curStr = curStr + nextByte;
        if(nextByte == '\n'){
          if(!isLaunchedPSRAM){
            if(previousRowsPSRAM.size() >= COMPACT_WALKBACK_COUNT){
              previousRowsPSRAM.erase(previousRowsPSRAM.begin());
            }
            previousRowsPSRAM.push_back(String(curStr));
          }
          curStr = "";
        }
      }
  }

  psramNextLoc = psram_memory_begin;
  PSRAMDumped = true;
}

// Returns whether the FRAM is initialized
bool isPSRAMReady(){
    return PSRAMReady;
}

// Returns whether the FRAM has been dumped
bool isPSRAMDumped(){
    return PSRAMDumped;
}

// Returns the next free memory location in FRAM
char* getPSRAMNextLoc(){
    return psramNextLoc;
}

// Returns true if next location is at 90% of total capacity
float getPSRAMCapacity(){
    return (psramNextLoc - psram_memory_begin) / (float)(psram_memory_end - psram_memory_begin);
}

// Adds the specified number of blank values to the CSV row. Argument is # of blank values to insert
void insertBlankValuesPSRAM(int numValues) {
  if (isPSRAMReady()) {
    for (int i = 0; i < numValues; i++) {
      psramPrint(F(",")); // Have blank data when sensor not found
    }
  }
}

void resetPSRAMDumpStatus(){
  PSRAMDumped = false;
}

void psramMarkLiftoff(){
  isLaunchedPSRAM = true;
}