
# ifndef DS3231_H
# define DS3231_H

# include <RTClib.h>
# include "RTC.h"

class DS3231: public RTC {
private: 
    RTC_DS3231 rtc;
    DateTime powerOnTime;
    DateTime launchTime;

public:
    DS3231(); // constructor
    void initialize(); 
    void onLaunch();
    double getTimeOn(); // ms
    double getTimeSinceLaunch(); // ms
    DateTime getLaunchTime(); 
    DateTime setLaunchTime();
    DateTime getPowerOnTime();
    DateTime getCurrentTime();
};

# endif