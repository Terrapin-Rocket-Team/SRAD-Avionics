#ifndef RADIO_MESSAGE_H
#define RADIO_MESSAGE_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__unix__) || defined(__APPLE__) // Windows, Linux, or OSX
#include <cstdint>
#endif

#include "Message.h"

#include "Data.h"
// pull Video classes
#include "Video/VideoData.h"
// pull APRS classes
#include "APRS/APRSData.h"
#include "APRS/APRSTelem.h"
#include "APRS/APRSCmd.h"
#include "APRS/APRSText.h"
// pull GroundStation classes
#include "GroundStation/GSData.h"

#endif