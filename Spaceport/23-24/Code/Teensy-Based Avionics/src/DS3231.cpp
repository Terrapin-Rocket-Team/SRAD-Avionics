# include <RTC.h>
#include "DS3231.h"


// constructor for the DS3231 class
DS3231::DS3231() {
   rtc = RTC_DS3231();
   powerOnTime = nullptr;
   launchTime = nullptr;
}

double DS3231::getTimeOn() {
    
}

double DS3231::getTimeSinceLaunch() {

}

void DS3231::initialize() {
    powerOnTime = rtc.now();
}

void DS3231::onLaunch() {
    launchTime = rtc.now();
}

DateTime DS3231::getPowerOnTime() {
    return powerOnTime;
}

DateTime DS3231::getLaunchTime() {
    return launchTime;
}

DateTime DS3231::getCurrentTime() {
    return rtc.now();
}