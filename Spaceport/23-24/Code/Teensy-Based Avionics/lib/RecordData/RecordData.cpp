#include "RecordData.h"

int PRE_FLIGHT_DATA_DUMP_DURATION = 60;  //in seconds
int PRE_FLIGHT_TIME_SINCE_LAST_DUMP = 0;  //in seconds
int PRE_FLIGHT_TIME_OF_LAST_DUMP = 0;  //in seconds

void recordData(char* data, char* stage){
    if(strcmp(stage, "Pre-Flight") == 0){
        dataToPSRAM(data);
        PRE_FLIGHT_TIME_SINCE_LAST_DUMP = (millis()/1000) - PRE_FLIGHT_TIME_OF_LAST_DUMP;
        if(PRE_FLIGHT_TIME_SINCE_LAST_DUMP > PRE_FLIGHT_DATA_DUMP_DURATION){
            String dumped = PSRAMDumpToSD();
            if(dumped == "Dumped"){
            resetPSRAMDumpStatus();
            PRE_FLIGHT_TIME_OF_LAST_DUMP = millis()/1000;
            }
        }
    }
    else if (strcmp(stage, "Post-Flight") == 0)
    {
        if (!isPSRAMDumped())
        {
            PSRAMDumpToSD();
        }
    }
    else if(stage[0] == 'S'){//'S' for "Stage X" TODO: implement better check for this lmao
        psramMarkLiftoff();  // TODO this should only have to run the first time
        dataToPSRAM(data);
    }
    else{
        // Serial.println("Flight Over Data Dumped");
       // TODO create error code here
    }
}

void dataToPSRAM(char* data){
    if(isPSRAMReady()){
        psramPrintln(data);
    }
}